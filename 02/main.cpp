#ifndef __PROGTEST__
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cassert>
#include <cmath>
using namespace std;
#endif /* __PROGTEST__ */
uint8_t * memdata; 
unsigned int memmax=0;		// cislo posledniho pouzitelneho data
unsigned int memused=0;		// cislo posledniho data ktere jiz neni mozne pouzit pro tabulku
unsigned int tablesize=0;	// velikost tabulky
bool memready=0;

// Vlozi cislo (4byty) na konec tabulky
bool   WriteToTable( const unsigned int& number)
{
	for(int byte=memmax-tablesize;byte>=memmax-tablesize-3;byte--)
	{
		if((byte<=memused)||(byte<0))
			return false;
		memdata[byte]=0x00;
		for(int bit=0;bit<8;bit++)
			memdata[byte]=memdata[byte] | ( ( (number>>( (8*(memmax-byte-tablesize))+bit ))&0x01 )<<bit );
	}
	tablesize+=4;
	return true;
}

// Vlozi cislo (4byty) na pozici offset
bool   WriteToTable( const unsigned int& number, const unsigned int& offset)
{
	for(unsigned int byte=offset;byte>=offset-3;byte--)
	{
		if((byte<=memused)||(byte<0))
			return false;
		memdata[byte]=0x00;
		for(unsigned int bit=0;bit<8;bit++)
			memdata[byte]=memdata[byte] | ( ( (number>>( (8*(offset-byte))+bit ))&0x01 )<<bit );
	}
	tablesize+=4;
	return true;
}

// Precte cislo (4byty) z pozice offset
unsigned int ReadFromTable( const unsigned int& offset )
{
	unsigned int value=0;
	for(unsigned int byte=offset;byte>=offset-3;byte--)
	{
		if((byte<=memused)||(byte<0))
			return 0;
		uint8_t TMP=memdata[byte];
		value|=(TMP<<(8*(offset-byte)));
		//printf("[VAL:%u]\n",value);
	}
	return value;
}

// Odstrani cisla (8bytu) z pozice offset
void   DeleteFromTable( unsigned int offset )
{
	unsigned int copy=offset-8;
	while (copy>memmax-tablesize)
	{
		memdata[offset]=memdata[copy];
		offset--;
		copy--;
	}
	tablesize-=8;
}

// Posune tabulku o 8bytu. Nemeni tablesize.
bool   MoveTable( unsigned int offset )
{
	unsigned int copy=memmax-tablesize-8;
	if((copy<=memused)||(copy<0))
		return false;
	while (copy<=offset)
	{
		memdata[copy]=memdata[copy+8];
		copy++;
	}
	return true;
}

// Vytiskne tabulky (pouze pro debugovaci uceli)
void   PrintTable ( void )
{
	printf("[TABLE size %u:",tablesize);
	for(unsigned int i=memmax;i>memmax-tablesize;i=i-8)
		printf("\n - offset:%u | size:%u | end:%u",ReadFromTable(i),ReadFromTable(i-4),ReadFromTable(i)+ReadFromTable(i-4)-1);
	printf("\n+ memused:%u]\n",memused);
}

// Jedoducha inicializace
void   HeapInit    ( void * memPool, int memSize )
{
	memdata=(uint8_t*)memPool;
	memmax=memSize-1;
	tablesize=0;
	memused=0;
	memready=1;
}

// Alokace
void * HeapAlloc   ( int    size)
{
	if(!memready)
		return NULL;
	unsigned int lastPOS=0;	// Prvni nevyuzity byte za blokem
	unsigned int thisPOS=0;	// Prvni vyuzity byte bloku
	for(int i=memmax;i>memmax-tablesize;i=i-8)
	{
		thisPOS=ReadFromTable(i);
		if((thisPOS-lastPOS)>=size)
		{
			printf("--- RESULT: %d --- SIZE: %d\n",thisPOS-lastPOS,size);
			printf("--- LASTPOS: %d --- THISPOS: %d\n",lastPOS,thisPOS);
			if((lastPOS+size-1)>memused)
				memused=lastPOS+size-1;
			if(MoveTable(i))
			{
				if(!WriteToTable(lastPOS,i))
					return NULL;
				if(!WriteToTable(size,i-4))
					return NULL;
				return (void*) (memdata+lastPOS);
			}
			else
			{
				return NULL;
			}
		}
		lastPOS=thisPOS+ReadFromTable(i-4);
	}
	if((memmax-tablesize-8)<=(memused+size))
		return NULL;
	else
	{
		if((lastPOS+size)>memused)
			memused=lastPOS+size-1;
		if(!WriteToTable(lastPOS))
			return NULL;
		if(!WriteToTable(size))
			return NULL;
		return (void*) (memdata+lastPOS);
	}
}

// Odstraneni
bool   HeapFree    ( void * blk )
{
	if(!memready)
		return false;
	uint8_t * findthis=(uint8_t*)blk;
	for(unsigned int i=memmax;i>memmax-tablesize;i=i-8)
	{
		if(findthis==(memdata+ReadFromTable(i)))
		{
			if(memused==(ReadFromTable(i)+ReadFromTable(i-4)-1))
			{
				if(tablesize>=16)
				{
					memused=ReadFromTable(i+4)+ReadFromTable(i+8)-1;
				}
				else
				{
					memused=0;
				}
			}
			DeleteFromTable(i);
			return true;
		}
	}
	return false;
}

// Konec
void   HeapDone    ( int  * pendingBlk )
{
	if(!memready)
		return;
	*pendingBlk=tablesize/8;
	memready=0;
}

#ifndef __PROGTEST__
int main ( void )
 {
   uint8_t       * p0, *p1, *p2, *p3, *p4;
   int             pendingBlk;
   static uint8_t  memPool[3 * 1048576];

   printf("\nTEST 1 -----------------------------\n");
   HeapInit ( memPool, 2097152 );
   PrintTable();
   assert ( ( p0 = (uint8_t*) HeapAlloc ( 512000 ) ) != NULL );
   PrintTable();
   memset ( p0, 0, 512000 );
   assert ( ( p1 = (uint8_t*) HeapAlloc ( 511000 ) ) != NULL );
   PrintTable();
   memset ( p1, 0, 511000 );
   assert ( ( p2 = (uint8_t*) HeapAlloc ( 26000 ) ) != NULL );
   PrintTable();
   memset ( p2, 0, 26000 );
   HeapDone ( &pendingBlk );
   assert ( pendingBlk == 3 );

   printf("\nTEST 2 -----------------------------\n");
   HeapInit ( memPool, 2097152 );
   PrintTable();
   assert ( ( p0 = (uint8_t*) HeapAlloc ( 1000000 ) ) != NULL );
   PrintTable();
   memset ( p0, 0, 1000000 );
   assert ( ( p1 = (uint8_t*) HeapAlloc ( 250000 ) ) != NULL );
   PrintTable();
   memset ( p1, 0, 250000 );
   assert ( ( p2 = (uint8_t*) HeapAlloc ( 250000 ) ) != NULL );
   PrintTable();
   memset ( p2, 0, 250000 );
   assert ( ( p3 = (uint8_t*) HeapAlloc ( 250000 ) ) != NULL );
   PrintTable();
   memset ( p3, 0, 250000 );
   assert ( ( p4 = (uint8_t*) HeapAlloc ( 50000 ) ) != NULL );
   PrintTable();
   memset ( p4, 0, 50000 );
   assert ( HeapFree ( p2 ) );
   PrintTable();
   assert ( HeapFree ( p4 ) );
   PrintTable();
   assert ( HeapFree ( p3 ) );
   PrintTable();
   assert ( HeapFree ( p1 ) );
   PrintTable();
   assert ( ( p1 = (uint8_t*) HeapAlloc ( 500000 ) ) != NULL );
   PrintTable();
   memset ( p1, 0, 500000 );
   assert ( HeapFree ( p0 ) );
   PrintTable();
   assert ( HeapFree ( p1 ) );
   PrintTable();
   HeapDone ( &pendingBlk );
   assert ( pendingBlk == 0 );

   printf("\nTEST 3 -----------------------------\n");
   HeapInit ( memPool, 2359296 );
   PrintTable();
   assert ( ( p0 = (uint8_t*) HeapAlloc ( 1000000 ) ) != NULL );
   PrintTable();
   memset ( p0, 0, 1000000 );
   assert ( ( p1 = (uint8_t*) HeapAlloc ( 500000 ) ) != NULL );
   PrintTable();
   memset ( p1, 0, 500000 );
   assert ( ( p2 = (uint8_t*) HeapAlloc ( 500000 ) ) != NULL );
   PrintTable();
   memset ( p2, 0, 500000 );
   assert ( ( p3 = (uint8_t*) HeapAlloc ( 500000 ) ) == NULL );
   PrintTable();
   assert ( HeapFree ( p2 ) );
   PrintTable();
   assert ( ( p2 = (uint8_t*) HeapAlloc ( 300000 ) ) != NULL );
   PrintTable();
   memset ( p2, 0, 300000 );
   assert ( HeapFree ( p0 ) );
   PrintTable();
   assert ( HeapFree ( p1 ) );
   PrintTable();
   HeapDone ( &pendingBlk );
   assert ( pendingBlk == 1 );

   printf("\nTEST 4 -----------------------------\n");
   HeapInit ( memPool, 2359296 );
   PrintTable();
   assert ( ( p0 = (uint8_t*) HeapAlloc ( 1000000 ) ) != NULL );
   PrintTable();
   memset ( p0, 0, 1000000 );
   assert ( ! HeapFree ( p0 + 1000 ) );
   PrintTable();
   HeapDone ( &pendingBlk );
   assert ( pendingBlk == 1 );

   printf("\nTEST 5 -----------------------------\n");
   HeapInit ( memPool, 1000000 );
   PrintTable();
   p0 = (uint8_t*) HeapAlloc ( 50000 );
   PrintTable();
   p1 = (uint8_t*) HeapAlloc ( 50000 );
   PrintTable();
   p2 = (uint8_t*) HeapAlloc ( 100000 );
   PrintTable();
   p3 = (uint8_t*) HeapAlloc ( 200000 );
   PrintTable();
   assert ( HeapFree ( p1 ) );
   PrintTable();
   HeapAlloc ( 33000 );
   PrintTable();
   HeapAlloc ( 33000 );
   PrintTable();
   HeapAlloc ( 33000 );
   PrintTable();
   assert ( HeapFree ( p3 ) );
   PrintTable();
   HeapAlloc ( 150000 );
   PrintTable();
   HeapAlloc ( 33000 );
   PrintTable();
   HeapAlloc ( 500000 );
   PrintTable();
   assert ( HeapFree ( p0 ) );
   PrintTable();
   HeapAlloc ( 25000 );
   PrintTable();
   HeapAlloc ( 25000 );
   PrintTable();
   return 0;
 }
#endif /* __PROGTEST__ */


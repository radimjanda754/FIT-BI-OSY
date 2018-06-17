#ifndef __PROGTEST__
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <vector>
#include <queue>
#include <stack>
#include <deque>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#if defined (__cplusplus) && __cplusplus > 199711L
/* C++ 11 */
#include <thread>
#include <mutex>     
#include <condition_variable>
#endif /* __cplusplus */
using namespace std;


struct TRect
 {
   int             m_X;
   int             m_Y;
   int             m_W;
   int             m_H;
 };

struct TCostProblem
 {
   int          ** m_Values;
   int             m_Size;
   int             m_MaxCost;
   void         (* m_Done) ( const TCostProblem *, const TRect * );
 };

struct TCrimeProblem
 {
   double       ** m_Values;
   int             m_Size;
   double          m_MaxCrime;
   void         (* m_Done) ( const TCrimeProblem *, const TRect * );
 };

#endif /* __PROGTEST__ */
#define BUFFER_SIZE 6
// Globalni promeny >:-[
int g_Pos = 0;
struct TBufferType
 {
   const TCostProblem * CoP;
   const TCrimeProblem * CiP;
   bool kill;
 };
TBufferType g_Buffer[BUFFER_SIZE];
pthread_mutex_t  g_Mutx, g_Waiter;
sem_t g_Full, g_Free;
const TCostProblem * (* GLOBcostFunct) (void);
const TCrimeProblem * (* GLOBcrimeFunct) (void);
int GLOBthrds;
// ----------------
bool               FindByCrime                             ( double          **,
                                                             int             ,
                                                             double          ,
                                                             TRect           *);
bool               FindByCost                              ( int             **,
                                                             int             ,
                                                             int             ,
                                                             TRect           *);


void * CrimeCreator ( )
 {
   while ( 1 )
    {
	  const TCrimeProblem * tmpCrime = GLOBcrimeFunct();
      sem_wait ( &g_Free );
      pthread_mutex_lock ( &g_Mutx );
	  if(tmpCrime==NULL)
		{
			pthread_mutex_unlock ( &g_Waiter );
			pthread_mutex_unlock ( &g_Mutx );
			break;
		}
	  g_Buffer[g_Pos] . CiP = tmpCrime;
      g_Buffer[g_Pos] . CoP = NULL;
      g_Pos ++;
      pthread_mutex_unlock ( &g_Mutx );
      sem_post ( &g_Full );
    }
   
   return ( NULL );
 }

void * CostCreator ( )
 {
   while ( 1 )
    {
	  const TCostProblem * tmpCost = GLOBcostFunct();
      sem_wait ( &g_Free );
      pthread_mutex_lock ( &g_Mutx );
	  if(tmpCost==NULL)
		{
			pthread_mutex_unlock ( &g_Mutx );
			pthread_mutex_lock ( &g_Waiter);
			for(int i=0;i<GLOBthrds;i++)
			{
				sem_wait ( &g_Free );
				pthread_mutex_lock ( &g_Mutx );
				g_Buffer[g_Pos].kill=1;
				g_Pos ++;
				pthread_mutex_unlock ( &g_Mutx);
				sem_post ( &g_Full );
			}
			pthread_mutex_unlock ( &g_Mutx);
			break;
		}
      g_Buffer[g_Pos] . CiP = NULL;
      g_Buffer[g_Pos] . CoP = tmpCost;
      g_Pos ++;
      pthread_mutex_unlock ( &g_Mutx);
      sem_post ( &g_Full );
    }
   
   return ( NULL );
 }

void * masterSolvers ( )
{
   while ( 1 )
    {
	  TRect DangerousRect;
	  DangerousRect.m_H=0;
	  DangerousRect.m_W=0;
      sem_wait ( &g_Full );
      pthread_mutex_lock ( &g_Mutx );
      g_Pos --;
		if(g_Buffer[g_Pos] . kill)
		{
		  pthread_mutex_unlock ( &g_Mutx );
		  sem_post ( &g_Free );
		  break;
		}
		if(g_Buffer[g_Pos] . CoP)
		{
			const TCostProblem * tmpCost = g_Buffer[g_Pos].CoP;
      		pthread_mutex_unlock ( &g_Mutx );
			FindByCost(tmpCost->m_Values,tmpCost->m_Size,tmpCost->m_MaxCost,&DangerousRect);
			if(DangerousRect.m_H==0||DangerousRect.m_W==0)
			tmpCost->m_Done(tmpCost,NULL);
			else
			tmpCost->m_Done(tmpCost,&DangerousRect);
		}
		else
		{
			const TCrimeProblem * tmpCrime = g_Buffer[g_Pos].CiP;
			pthread_mutex_unlock ( &g_Mutx );
			FindByCrime(tmpCrime->m_Values,tmpCrime->m_Size,tmpCrime->m_MaxCrime,&DangerousRect);
			if(DangerousRect.m_H==0||DangerousRect.m_W==0)
			tmpCrime->m_Done(tmpCrime,NULL);
			else
			tmpCrime->m_Done(tmpCrime,&DangerousRect);
		}
      sem_post ( &g_Free );

      //printf ( "Consumer %d received %d from %d\n", (int)id, val, from );
    }
	return NULL;
}



void               MapAnalyzer                             ( int               threads,
                                                             const TCostProblem * (* costFunc) ( void ),
                                                             const TCrimeProblem* (* crimeFunc) ( void ) )
 {
	GLOBthrds=threads;
	GLOBcostFunct = costFunc;
	GLOBcrimeFunct = crimeFunc;
	g_Pos=0;
	pthread_t      * thrID;
	pthread_attr_t   attr;
	thrID = (pthread_t *) malloc ( sizeof ( *thrID ) * ( threads + 2 ) );
	pthread_attr_init ( &attr );
	pthread_attr_setdetachstate ( &attr, PTHREAD_CREATE_JOINABLE );
	pthread_mutex_init ( &g_Waiter, NULL );
	pthread_mutex_init ( &g_Mutx, NULL );
	sem_init ( &g_Free, 0, BUFFER_SIZE );
	sem_init ( &g_Full, 0, 0 );

	pthread_mutex_lock ( &g_Waiter );
	for(int i = 0; i < BUFFER_SIZE; i++)
		g_Buffer[i].kill=0;

   for (int i = 0; i < threads; i ++ )
    if ( pthread_create ( &thrID[i], &attr, (void*(*)(void*)) masterSolvers, NULL ) )
     {
       perror ( "pthread_create" );
       return;    
     }

    if ( pthread_create ( &thrID[threads], &attr, (void*(*)(void*)) CrimeCreator, NULL ) )
     {
       perror ( "pthread_create" );
       return;    
     }

    if ( pthread_create ( &thrID[threads+1], &attr, (void*(*)(void*)) CostCreator, NULL ) )
     {
       perror ( "pthread_create" );
       return;    
     }

   pthread_attr_destroy ( &attr );  
   for (int i = 0; i < threads+2; i ++ )
    pthread_join ( thrID[i], NULL );  
   pthread_mutex_destroy ( &g_Mutx );
   pthread_mutex_destroy ( &g_Waiter );
   sem_destroy ( &g_Free );
   sem_destroy ( &g_Full );
   free ( thrID );
 }

bool               FindByCost                              ( int            ** values,
                                                             int               size,
                                                             int               maxCost,
                                                             TRect           * res )
 {
	// SPECIALNI PRIPAD
	if(size==1)
	{
		if(values[0][0]<=maxCost)
			{
				res->m_X=1;res->m_Y=1;res->m_W=1;res->m_H=1;
				return true;
			}
		return false;
	}
	// DEKLARACE POMOCNE MATICE
	int maxsize=0;	
	int** arr = new int*[size];
	for(int i = 0; i < size; ++i)
		arr[i] = new int[size];
	// PRINT INPUT
	/*for(int iY = size-1; iY >= 0; --iY)
	{
		for(int iX = 0; iX < size; ++iX)
			printf("%d ",values[iY][iX]);
		printf("\n");
	}*/
	// PROCHAZENI CELE MATICE
	for(int iX = 0; iX < size; ++iX)
		for(int iY = 0; iY < size; ++iY)
			{
				// MINOR SPEED BOOST
				if(((size-iY)*(size-iX))<maxsize)
					continue;
				// VYPLNENI OKRAJU
				arr[iY][iX]=values[iY][iX];
				if(maxsize<1&&values[iY][iX]<=maxCost)
				{
					res->m_X=iX;res->m_Y=iY;res->m_W=1;res->m_H=1;
					maxsize=1;
				}
				for(int ADD = iX+1; ADD < size; ++ADD)
					{
						arr[iY][ADD]=values[iY][ADD]+arr[iY][ADD-1];
						if((ADD-iX+1)>maxsize&&arr[iY][ADD]<=maxCost)
						{
							res->m_X=iX;res->m_Y=iY;res->m_W=ADD-iX+1;res->m_H=1;
							maxsize=ADD-iX+1;
						}
					}	
				for(int ADD = iY+1; ADD < size; ++ADD)
					{
						arr[ADD][iX]=values[ADD][iX]+arr[ADD-1][iX];
						if((ADD-iY+1)>maxsize&&arr[ADD][iX]<=maxCost)
						{
							res->m_X=iX;res->m_Y=iY;res->m_W=1;res->m_H=ADD-iY+1;
							maxsize=ADD-iY+1;
						}
					}
				// POCITANI VNITRNI CASTI
				for(int ADDX = iX+1; ADDX < size; ++ADDX)								
					for(int ADDY = iY+1; ADDY < size; ++ADDY)
						{
							arr[ADDY][ADDX]=values[ADDY][ADDX]+arr[ADDY][ADDX-1]+arr[ADDY-1][ADDX]-arr[ADDY-1][ADDX-1];
							if(((ADDY-iY+1)*(ADDX-iX+1))>maxsize&&arr[ADDY][ADDX]<=maxCost)
							{
								maxsize=(ADDY-iY+1)*(ADDX-iX+1);
								res->m_X=iX;res->m_Y=iY;res->m_W=ADDX-iX+1;res->m_H=ADDY-iY+1;								
							}
						}
				// LADENI
				/*if(iX==0&&iY==0)
				{
					printf("================COUNTER===========\n");
					for(int iY = size-1; iY >= 0; --iY)
					{
						for(int iX = 0; iX < size; ++iX)
							printf("%d ",arr[iY][iX]);
						printf("\n");
					}
				}*/
			}


	for(int i = 0; i < size; ++i)
		delete[] arr[i];
	delete[] arr;
	if(maxsize>0)
		return true;
	return false;
 }

bool               FindByCrime                             ( double         ** values,
                                                             int               size,
                                                             double            maxCrime,
                                                             TRect           * res )
 {
	// SPECIALNI PRIPAD
	if(size==1)
	{
		if(values[0][0]<=maxCrime)
			{
				res->m_X=1;res->m_Y=1;res->m_W=1;res->m_H=1;
				return true;
			}
		return false;
	}
	// DEKLARACE POMOCNE MATICE
	int** arr = new int*[size];
	for(int i = 0; i < size; ++i)
		arr[i] = new int[size];
	// VYTVORI HISTOGRAMY
	for(int iX = 0; iX < size; ++iX)
	{
		int increase=0;
		for(int iY = 0; iY < size; ++iY)
			{
				if(values[iY][iX]>maxCrime)
				{
					arr[iY][iX]=0;
					increase=0;
				}
				else
				{
					arr[iY][iX]=++increase;
				}
			}
	}
	// DEBUG VYPIS
	/*for(int iY = size-1; iY >= 0; --iY)
	{
		for(int iX = 0; iX < size; ++iX)
			printf("%d ",arr[iY][iX]);
		printf("\n");
	}*/
	// ZISKEJ MAX VELIKOST V HISTOGRAMU
	int TotalMaxSize=0;
	int OnRow=0;
	for(int iY=size-1; iY >= 0; --iY)
	{
		if(TotalMaxSize>((iY+1)*size))
			break;
		stack<int> st;
		int maxsize = 0;
		int stTOP=0;
		int areaTOP=0;
		int i=0;
		while(i<size)
		{
			if (st.empty() || arr[iY][st.top()] <= arr[iY][i])
				st.push(i++);
			else
			{
			    stTOP = st.top();
			    st.pop();
				int tmp;
				if(st.empty()) tmp=i;else tmp=i-st.top()-1;
			    areaTOP = arr[iY][stTOP] * tmp;
			    if (maxsize < areaTOP)
			        maxsize = areaTOP;
			}
		}
		while (st.empty() == false)
		{
		    stTOP = st.top();
		    st.pop();
			int tmp;
			if(st.empty()) tmp=i;else tmp=i-st.top()-1;
		    areaTOP = arr[iY][stTOP] * tmp;
		    if (maxsize < areaTOP)
		        maxsize = areaTOP;
		}
		if(maxsize>TotalMaxSize)
		{
			TotalMaxSize=maxsize;
			OnRow=iY;
		}
	}
	if(TotalMaxSize==0)
		return false;
	// VYHLEDANI POZICE
	int leastElem=0;
	for(int X1=0;X1<size;X1++)
	{
		//printf("\nStarting from: %d ",X1);
		if(arr[OnRow][X1]==0)
			continue;
		//printf("[1SUM:%d]",Sum);
		leastElem=arr[OnRow][X1];
		//printf("[1LEM:%d]",leastElem);
		if(arr[OnRow][X1]==TotalMaxSize)
		{
			res->m_H=leastElem;
			res->m_W=1;
			res->m_X=X1;
			res->m_Y=OnRow-leastElem+1;
			break;
		}
		for(int X2=X1+1;X2<size;X2++)
			{
				if(arr[OnRow][X2]==0)
					break;
				//printf("[2SUM:%d]",Sum);
				if(arr[OnRow][X2]<leastElem)
				{
					leastElem=arr[OnRow][X2];
					//printf("[2LEM:%d]",leastElem);
				}
				if(((X2-X1+1)*leastElem)==TotalMaxSize)
				{
					res->m_H=leastElem;
					res->m_W=X2-X1+1;
					res->m_X=X1;
					res->m_Y=OnRow-leastElem+1;
					X1=size;break;
				}
			}
	}
	// KONEC
	for(int i = 0; i < size; ++i)
		delete[] arr[i];
	delete[] arr;
	if(TotalMaxSize>0)
		return true;
   return false;
 }


























Úkolem je realizovat sadu funkcí, které budou umožňovat rychle naceňovat pozemky podle různých kritérií.

Dnešní burzovní operace jsou závislé na rychlém zpracování a rozhodování. Úkolem bude vytvořit takovou podporu pro spekulace s pozemky. Předpokládáme, že máme mapu pozemků k prodeji/koupi. Tato mapa má čtvercový tvar a je rozdělena na N x N čtvercových polí (parcel). Parcela je dále nedělitelná a má pro další úvahu jednotkovou velikost. Chceme vyhledávat obdélníkový výřez mapy (tedy X x Y parcel, X ≤ N, Y ≤ N) podle následujících kritérií:

nalezení co největší části mapy, kde celková cena parcel je nižší nebo rovná zadané mezi. Na vstupu je mapa NxN parcel, pro každou parcelu známe její cenu. Cena může být kladná i záporná (na parcele je závazek). Úkolem je najít obdélníkový výřez s co největší plochou, kde celková částka nepřekračuje zadanou mez. Takových výřezů může být i více, úkolem je najít alespoň jeden či nahlásit, že za danou cenu nejde nic koupit.
Nalezení co největší části mapy, kde míra kriminality na žádné parcele nepřekračuje zadanou mez. Na vstupu je mapa NxN parcel, pro každou parcelu známe hodnotu zločinnosti (průměrný počet zločinů spáchaných v daném místě během nějakého sledovaného období, nezáporné desetinné číslo). Úkolem je najít obdélníkový výřez s co největší plochou, kde na žádné parcele výřezu nepřekročí míra kriminality zadanou mez. Takových výřezů může být opět více, úkolem je najít alespoň jeden či nahlásit, že za daných podmínek nelze nic koupit.
Vaším úkolem je realizovat software, který dokáže tyto problémy řešit. Protože se jedná o výpočetně náročné problémy, je potřeba výpočet rozdělit do vláken a tím výpočet urychlit. Požadované rozhraní má následující podobu:

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

void MapAnalyzer ( int               threads,
                   const TCostProblem * (* costFunc) ( void ),
                   const TCrimeProblem * (* crimeFunc) ( void ) );

bool FindByCost  ( int            ** values,
                   int               size,  
                   int               maxCost,
                   TRect           * res );  

bool FindByCrime ( double         ** values,
                   int               size,  
                   double            maxCrime,
                   TRect           * res );   
TRect
je pomocná struktura popisující obdélníkový výřez v mapě. Má složky m_X, m_Y, m_W a m_H, tyto udávají pozici levého horního rohu, šířky a výšky.
TCostProblem
struktura popisuje problém hledání největšího výřezu s celkovou cenou menší než zadaná mez. Složky struktury jsou:
m_Values - 2D pole hodnot s cenou jednotlivých parcel. Pole je orientováno po řádcích, tedy pozice x,y bude na indexech m_Values[y][x] (nejprve řádek - y),
m_Size - velikost mapy (mapa je čtvercová),
m_MaxCost - maximální celková cena,
m_Done - ukazatel na funkci, kterou je potřeba zavolat po vyřešení problému s předanými výsledky.
Význam složek struktury je jasný, jediný problém může být se složkou m_Done. Předpokládáme asynchronní model výpočtu. Volající (zadavatel) vyplní složky struktury a předá problém Vaší implementaci k vyřešení. Funkce nezačne problém řešit hned, nejspíše jej předá nějakému vláknu ke zpracování, možná jej i rozdělí, ... V okamžiku, kdy bude výpočet hotový, je potřeba řešení předat zadavateli. To Vaše implementace provede tím, že zavolá právě tuto funkci. Při volání je potřeba předat 2 parametry: prvním je zadání vyřešeného problému (předáte ukazatel na TCostProblem v nezměněné podobě) a druhým je ukazatel na vyplněnou strukturu, kde se nachází hledaný výřez mapy. Pokud se žádný výřez nepodaří najít (každá jednotlivá parcela je dražší než zadaná mez), bude druhý parametr volání NULL.
TCrimeProblem
je struktura popisující hledání výřezu mapy, kde kriminalita na žádné parcele nepřekračuje zadanou mez. Složky jsou:
m_Values - 2D pole hodnot s mírou kriminality pro jednotlivé parcely, pole je opět uloženo po řádcích,
m_Size - velikost mapy (mapa je čtvercová),
m_MaxCrime - maximální přípustná míra kriminality na jednotlivé parcele,
m_Done - ukazatel na funkci, kterou je potřeba zavolat po vyřešení problému s předanými výsledky, použití je shodné jako v předchozí struktuře.
MapAnalyzer(threads, costFunc, crimeFunc)
Vámi implementovaná funkce, která umožní časově optimální řešení problémů vyhledávání v mapách. Funkce bude zodpovědná za vytvoření čtecích a pracovních vláken, jejich synchronizaci a ukončení. Parametry funkce jsou:
threads - počet pracovních vláken, které má funkce vytvořit. Funkce vytvoří tato vlákna, aby fakticky počítala zadané problémy. Dále funkce vytvoří 1-2 vlákna pro načítání vstupů (viz níže).
costFunc - ukazatel na funkci, kterou bude volat jedno z načítacích vláken. Po zavolání takto zprostředkované funkce dostane volající ukazatel odkazující na další problém k vyřešení. Tento ukazatel si nejspíše předáte pracovním vláknům, aby jej dále zpracovala a vyřešila. Pozor: takto předaný ukazatel je určený pouze ke čtení, je potřeba jej odevzdat funkci m_Done v nezměněné podobě (původní obsah i původní adresa, ne kopie dat). Pokud volání costFunc vrátí NULL, znamená to, že další problémy typu "největší oblast s danou maximální cenou" již nebudou zadávané (ale stále ještě mohou být zadávané problémy s kriminalitou).
crimeFunc - ukazatel na funkci, která po zavolání vrací hodnotu ukazatele odkazujícího na další problém typu "největší oblast s max. kriminalitou nepřevyšující danou mez". Pro funkci platí stejné jako pro costFunc.
Bylo zmíněno, že funkce MapAnalyzer vytvoří threads+2 vláken - dvě vlákna budou načítat vstup voláním předaných ukazatelů na funkce. Fakticky ale stačí vytvořit pouze threads + 1 nových vláken a využít i vlákna. ze kterého byla volaná funkce MapAnalyzer.

Funkce MapAnalyzer skončí v okamžiku, kdy byly vyřešené všechny zadané vstupní problémy a další nové problémy nejsou k dispozici (jak costFunc, tak crimeFunc vrací NULL). Po zaniknutí Vámi vytvořených vláken funkce MapAnalyzer vrátí řízení volanému. Neukončujte celý program (nevolejte exit a podobné funkce), pokud ukončíte celý program, budete hodnoceni 0 body.

FindByCost(map, size, maxCost, res)
Vámi implementovaná funkce, která sekvenčně nalezne největší výřez mapy, kde celková cena parcel nepřekračuje zadanou mez. Parametry jsou:
values - cena jednotlivých parcel na mapě, analogie složky TCostProblem . m_Values,
size - velikost mapy, odpovídá TCostProblem . m_Size,
maxCost - maximální celková cena, TCostProblem . m_MaxCost,
res - výstupní parametr, kam funkce umístí nalezenou oblast na mapě splňující zadání.
Návratovou hodnotou funkce je true pro úspěch (oblast na mapě nalezena, výstupní parametr res obsahuje platná data), nebo false pokud se nepodaří takový výřez mapy najít (zadaná max. cena je příliš nízká).

Tato funkce je testovaná v prvním testu (sekvenční). Slouží k tomu, abyste mohli snáze nalézt případné chyby ve Vašem algoritmu hledání výřezu mapy podle ceny.

FindByCrime(map, size, maxCrime, res)
je analogie funkce FindByCost pro řešení problému nalezení výřezu mapy, kde max. kriminalita nepřekračuje zadanou mez. Funkce opět slouží ke snazšímu otestování implementace vyhledávání.
Odevzdávejte zdrojový kód s implementací požadovaných funkcí. Do Vaší implementace nevkládejte funkci main ani direktivy pro vkládání hlavičkových souborů. Funkci main a hlavičkové soubory lze ponechat pouze v případě, že jsou zabalené v bloku podmíněného překladu.

Využijte přiložené ukázkové soubory. Zdrojové soubory a Makefile lze použít pro lokální testování Vaší implementace. Celá implementace patří do souboru solution.cpp, dodaný soubor je pouze mustr. Pokud zachováte bloky podmíněného překladu, můžete soubor solution.cpp odevzdávat jako řešení úlohy.

Při řešení lze využít C++11 API pro práci s vlákny (viz vložené hlavičkové soubory). Dostupný kompilátor však nezvládá C++11 ideálně. Pokud při použití C++11 konstrukcí či API narazíte na chybu/nedokonalost kompilátoru, budete muset svůj program přepsat tak, abyste tuto chybu či nedokonalost obešli. Použitý kompilátor je g++ verze 4.7.

Nápověda:
Nejprve implementujte sekvenční funkce pro vyhledávání na mapě. Správnost implementace lze ověřit lokálně pomocí infrastruktury v přiloženém archivu. Až budete mít funkce lokálně otestované, můžete je zkusit odevzdat na Progtest (pro tento pokus nechte MapAnalyzer s prázdným tělem). Takové řešení samozřejmě nedostane žádné body, ale uvidíte, zda správně projde sekvenčními testy.
Abyste zapojili co nejvíce jader, zpracovávejte několik problémů najednou. Vyzvedněte je pomocí opakovaného volání costFunc/crimeFunc a okamžitě po analýze vracejte odpověď pomoci m_Done. Není potřeba dodržovat pořadí při odevzdávání. Pokud budete najednou zpracovávat pouze jeden problém, nejspíše zaměstnáte pouze jedno vlákno a ostatní vlákna budou čekat bez užitku.
Funkce MapAnalyzer je volaná opakovaně, pro různé vstupy. Nespoléhejte se na inicializaci globálních proměnných - při druhém a dalším zavolání budou mít globální proměnné hodnotu jinou. Je rozumné případné globální proměnné vždy inicializovat na začátku funkce MapAnalyzer. Ještě lepší je nepoužívat globální proměnné vůbec.
Nepoužívejte mutexy a podmíněné proměnné inicializované pomocí PTHREAD_MUTEX_INITIALIZER, důvod je stejný jako v minulém odstavci. Použijte raději pthread_mutex_init().
Testovací prostředí samo o sobě nevytváří žádná vlákna, tedy funkce MapAnalyzer sama o sobě nemusí být reentrantní (může používat globální proměnné, s omezením výše).
Mapy a struktury popisující zadávané problémy alokovalo testovací prostředí. Testovací prostředí se také postará o jejich uvolnění (po převzetí funkcí m_Done). Jejich uvolnění tedy není Vaší starostí. Váš program je ale zodpovědný za uvolnění všech ostatních prostředků, které si alokoval.
Problémy musíte načítat, zpracovávat a odevzdávat průběžně. Postup, kdy si všechny problémy načtete do paměťových struktur a teprve pak je začnete zpracovávat, nebude fungovat. Takové řešení skončí deadlockem v prvním testu s více vlákny. Musíte zároveň obsluhovat požadavky typu Cost i Crime. Řešení, které se bude snažit nejprve vyřešit všechny problémy typu Cost a pak začne obsluhovat problémy Crime, skončí taktéž deadlockem.
Volání m_Done je reentrantní, není potřeba je serializovat (obalovat mutexy). Každý vyřešený problém odevzdávejte právě 1x. Rozumné je volat funkci m_Done přímo z výpočetního vlákna, které pro daný problém dokončilo analýzu.
Neukončujte funkci MapAnalyzer pomocí exit, pthread_exit a podobných funkcí. Pokud se funkce MapAnalyzer nevrátí do volajícího, bude Vaše implementace vyhodnocena jako nesprávná.
Využijte přiložená vzorová data. V archivu jednak naleznete ukázku volání rozhraní (práce s ukazateli na funkce) a dále několik testovacích vstupů a odpovídajících výsledků.
Nebojte se ukazatelů na funkce. Předaný parametr, např. costFunc použijte při volání jako každou jinou funkci:
    void MapAnalyzer ( ..., const TCostProblem * (*costFunc) ( void ), ... )
     {
       ...
       const TCostProblem * x = costFunc ( );
       ...
     } 
   
Pokud chcete volat funkci zprostředkovanou ukazatelem costFunc z jiné funkce než z MapAnalyzer, musíte si ukazatel předat:
    void foo ( void )
     {
       // nefunguje, funkce s takovym jmenem (asi) v testovacim prostredi
       // neexistuje 
       x = costFunc ( ); 
     }
    void bar ( const TCostProblem * (* costProblemFunc) ( void ) )
     {
       x = costProblemFunc (); // ok
     }
    void MapAnalyzer ( ..., const TCostProblem * (*costFunc) ( void ), ... )
     {
       ...
       foo ();
       bar ( costFunc );
       ...
     } 
   
V testovacím prostředí je k dispozici STL. Myslete ale na to, že ten samý STL kontejner nelze najednou zpřístupnit z více vláken. Více si o omezeních přečtěte např. na C++ reference - thread safety.
Testovací prostředí je omezené velikostí paměti. Není uplatňován žádný explicitní limit, ale VM, ve které testy běží, je omezena 1 GiB celkové dostupné RAM. Úloha může být dost paměťově náročná, zejména pokud se rozhodnete pro jemné členění úlohy na jednotlivá vlákna. Pokud se rozhodnete pro takové jemné rozčlenění úlohy, možná budete muset přidat synchronizaci běhu vláken tak, aby celková potřebná paměť v žádný nepřesáhla nějaký rozumný limit. Pro běh máte garantováno, že Váš program má k dispozici nejméně 200 MiB pro Vaše data (data segment + stack + heap). Pro zvídavé - zbytek do 1GiB je zabraný běžícím OS, dalšími procesy, zásobníky Vašich vláken a nějakou rezervou.
Co znamenají jednotlivé testy:
Test algoritmu (sekvencni)
Testovací prostředí opakovaně volá funkci FindByCost/FindByCrime pro různé vstupy a kontroluje vypočtené výsledky. Slouží pro otestování implementace Vašeho algoritmu. Funkce MapAnalyzer není v tomto testu vůbec volaná. Zároveň si na tomto testu můžete ověřit, zda Vaše implementace algoritmu je dostatečně rychlá.
Základní test/test několika/test mnoha thready
Testovací prostředí volá funkci MapAnalyzer pro různý počet vláken.
Test zahlcení
Testovací prostředí generuje velké množství požadavků a kontroluje, zda si s tím Vaše implementace poradí. Pokud nebudete rozumně řídit počet rozpracovaných požadavků, překročíte paměťový limit.
Test zrychleni vypoctu
Testovací prostředí spouští Vaši funkci pro ta samá vstupní data s různým počtem vláken. Měří se čas běhu (wall i CPU). S rostoucím počtem vláken by měl wall time klesat, CPU time mírně růst (vlákna mají možnost běžet na dalších CPU). Pokud wall time neklesne, nebo klesne málo (např. pro 2 vlákna by měl ideálně klesnout na 0.5, existuje určitá rezerva), test není splněn.
Busy waiting - pomale pozadavky
Do volání costFunc/crimeFunc testovací prostředí vkládá uspávání vlákna (např. na 100 ms). Výpočetní vlákna tím nemají práci. Pokud výpočetní vlákna nejsou synchronizovaná blokujícím způsobem, výrazně vzroste CPU time a test selže.
Busy waiting - pomale notifikace
Do volání m_Done je vložena pauza. Pokud jsou špatně blokována vlákna načítající vstup, výrazně vzroste CPU time. (Tento scénář je zde méně pravděpodobný.) Dále tímto testem neprojdete, pokud zbytečně serializujete volání m_Done.
Busy waiting - complex
Je kombinací dvou posledně jmenovaných testů.
Test rozlozeni zateze
Testovací prostředí zkouší výpočet pouze pro jeden druh problémů (cost/crime). Výpočet by měl být rozdělen mezi všechna dostupná vlákna, vlákna by neměla být určena pro řešení problému pouze jednoho typu. Pokud není výpočet dostatečně zrychlen, test selže. Test není povinný, ale jeho nezvládnutí znamená citelný bodový postih.
Test rozlozeni zateze (jemny)
Testovací prostředí zkouší, zda se do řešení jednoho problému dokáže zapojit více dostupných vláken. Pokud chcete v tomto testu uspět, musíte Váš program navrhnout tak, aby bylo možné využít více vláken i při analýze jednoho vstupního problému. Jedná se o test bonusový.
Jak to vyřešit - pozor, SPOILER
Pokud se nechcete obrat o dobrý pocit, že jste úlohu vyřešili zcela sami, nečtěte dále.

Problém hledání oblasti s celkovou cenou nepřevyšující zadanou mez lze řešit v čase O(n^4), kde n je šířka/výška mapy. Vzhledem k možným záporným hodnotám na vstupu nelze složitost snížit.
Problém hledání oblasti s max. kriminalitou nepřevyšující danou mez lze řešit se složitostí O(n^3), kde n je šířka/výška mapy. Této složitosti lze dosáhnout s minimem triků. Pokud je využito dynamické programování, lze dosáhnout i složitosti O(n^2). Testovací prostředí primárně počítá s kubickým algoritmem řešení. Pokud detekuje kvadratické řešení, upraví patřičně rozsah vstupních dat. Časy v ukázce odpovídají kubickému algoritmu.
Rozdílná časová složitost problémů byla zvolena úmyslně, abyste byli nuceni dynamicky rozdělovat požadavky na výpočetní vlákna.
Vzhledem k heterogennímu charakteru vstupních dat se hodí objektový návrh s polymorfismem.
Další nápověda - SUPERSPOILER
Podle potřeby v průběhu řešení úlohy zveřejníme další nápovědy pro tápající studenty.
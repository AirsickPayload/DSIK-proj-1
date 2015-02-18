#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>

void wyslijPlik(int sdsocket, char *nazwa)
{
	char filename[512],bufor[1024];

	strcpy(filename,nazwa);

	if(send(sdsocket,filename, sizeof(filename),0) <= 0)
	{
		printf("Potomny: blad poczas wysylania nazwy pliku - koncze dzialanie!");
		close(sdsocket); return;
	}

	struct stat fileinfo;
    FILE* plik;

    if (stat(nazwa, &fileinfo) < 0)
    {
        printf("Potomny: nie moge pobrac informacji o pliku\n");
        close(sdsocket); return;
    }
	
	long dlpliku,wyslano,wyslano_razem,przeczytano;
	dlpliku = htonl((long) fileinfo.st_size);

	if(send(sdsocket, &dlpliku, sizeof(long), 0) <= 0)
	{
		printf("Potomny: blad podczas wysylanie wielkosci pliku");
		close(sdsocket); return;
	}
	
	dlpliku = fileinfo.st_size;
	wyslano_razem=0;
	if((plik = fopen(nazwa,"rb")) == 0)
	{
		printf("Blad podczas otwierania pliku!");
		close(sdsocket); return;
	}

    while (wyslano_razem < dlpliku)
    {
		memset(bufor,0,1024);
        przeczytano = fread(bufor, 1, 1024, plik);
        wyslano = send(sdsocket, bufor, przeczytano, 0);
        if (przeczytano != wyslano)
		{
			printf("Potomny: blad poczas wysylania pliku - koncze dzialanie!"); close(sdsocket); exit(1);
		}
        wyslano_razem += wyslano;
    }
	fclose(plik);
	char potw;
	recv(sdsocket,&potw,sizeof(potw),0);
	if (potw == 1)
        printf("Potomny: plik wyslany poprawnie\n");
    else
		{
        	printf("Potomny: blad przy wysylaniu pliku\n");
			close(sdsocket); exit(1);
		}
}

void odbiorPliku(int gniazdoK, char *filenameDest)
{
	char filename[512], bufor[1024];
	long rozmiar,odebrano,odebrano_razem;
	FILE *plik;
	char potw;

	memset(filename,0,512);
	odebrano_razem = 0;

	/* odbior nazwy pliku */
	if( recv(gniazdoK, filename, sizeof(filename), 0) != sizeof(filename) ) 
	{ 
		printf("Potomny: blad podczas odbierania nazwy pliku."); close(gniazdoK); return; 
	}
	/*odbior rozmiaru pliku */
	if( recv(gniazdoK, &rozmiar, sizeof(long), 0) != sizeof(long) ) 
	{ 
		printf("Potomny: blad podczas odbierania rozmiaru pliku."); close(gniazdoK); return; 
	}
	rozmiar = ntohl(rozmiar);
	printf("Potomny: Nastapi odebranie pliku o nazwie %s, o rozmiarze %ld bajtow.\n",filename, rozmiar);
	
	printf("Potomny: Rozpoczeto odbieranie bajtow pliku...\n");
	if((plik = fopen(filename,"wb")) == 0)
	{
		printf("Blad podczas otwierania pliku!");
		close(gniazdoK); return;
	}

	while(odebrano_razem < rozmiar)
	{
		memset(bufor,0,1024);
		odebrano = recv(gniazdoK, bufor, sizeof(bufor), 0);
		if(odebrano < 0) 
		{
			printf("Potomny: blad poczas odbierania pliku - koncze dzialanie!"); close(gniazdoK); exit(1);
		}
		odebrano_razem += odebrano;
		fwrite(bufor, 1, odebrano, plik);
	}
	fclose(plik);

    if (odebrano_razem != rozmiar)
	{
        printf("*** BLAD W ODBIORZE PLIKU %s ***\n",filename);
		potw = 0;
		send(gniazdoK,&potw,sizeof(potw),0);
		close(gniazdoK); return;
	}
    else
	{
        printf("*** PLIK %s ODEBRANY POPRAWNIE ***\n",filename);	
		potw = 1;
		send(gniazdoK,&potw,sizeof(potw),0);
	}
	strcpy(filenameDest, filename);
}

void obslugaKlienta(int gniazdoK)
{
	short ile;
	int i;
	char **tabPlikow;
	char parametry[256];

	if( recv(gniazdoK,&ile,sizeof(short),0) <=0)
    {
        printf("Potomny: blad przy odbiorze liczby plikow.\n");
        return;
    }
	else
	{
		ile = ntohs(ile);
		printf("Potomny: Zadeklarowana liczba plikow przez klienta: %d.\n", ile);
	}

	/* inicjalizacja dwuwymiarowej tablicy typu char przechowujacej nazwy plikow */
	tabPlikow = malloc(sizeof *tabPlikow * ile);
	if (tabPlikow)
	{
	  for (i = 0; i<ile; i++)
	  {
		tabPlikow[i] = malloc(sizeof *tabPlikow[i] * 512);
	  }
	}

	for(i = 0; i<ile; i++)
	{
		odbiorPliku(gniazdoK,tabPlikow[i]);
	}

	memset(parametry,0,256);
	if( recv(gniazdoK,parametry,sizeof(parametry),0) <=0)
    {
        printf("Potomny: blad przy odbiorze parametrow.\n");
        close(gniazdoK); return;
    }

	char ** tabOdebrParam  = NULL;
	char *  p = strtok(parametry, " ");
	int n_spacji = 0;

	while (p) 
	{
	  tabOdebrParam = realloc (tabOdebrParam, sizeof (char*) * ++n_spacji);

	  if (tabOdebrParam == NULL)
		exit (1);

	  tabOdebrParam[n_spacji-1] = p;

	  p = strtok (NULL, " ");
	}

	tabOdebrParam = realloc(tabOdebrParam, sizeof (char*) * (n_spacji+1));
	tabOdebrParam[n_spacji] = 0;

	pid_t child_pid;
	int status;
	
	/* budowanie nazwy archiwum na podstawie nazwy pierwszego odebranego pliku i pierwszego parametru(zawierajacego format archiwum) */
	char nazwaArchiwum[512];
	memset(nazwaArchiwum,0,512);

	strcpy(nazwaArchiwum,tabPlikow[0]);
	strcat(nazwaArchiwum,"."); strcat(nazwaArchiwum,tabOdebrParam[0]+2);	
	printf("Potomny: Rozpoczynam tworzenie archiwum %s.\n", nazwaArchiwum);
	/* do utworzenia archiwum zostaje wykorzystany kolejny proces potomny, ponieważ po poprawnym uruchomieniu polecenia przy pomocy exec* sterowanie nie wraca juz do programu */
	for(i = 0; i<ile; i++)
	{
		if((child_pid=fork()) == 0)
		{
			if(n_spacji == 1)
			execlp("7z", "7z", "a", tabOdebrParam[0], nazwaArchiwum, tabPlikow[i], NULL);
			else if(n_spacji == 2)
			execlp("7z", "7z", "a", tabOdebrParam[0], tabOdebrParam[1], nazwaArchiwum, tabPlikow[i], NULL);
			else if(n_spacji == 3)
			execlp("7z", "7z", "a", tabOdebrParam[0], tabOdebrParam[1], tabOdebrParam[2], nazwaArchiwum, tabPlikow[i], NULL);
			else if(n_spacji == 4)
			execlp("7z", "7z", "a", tabOdebrParam[0], tabOdebrParam[1], tabOdebrParam[2], tabOdebrParam[3], nazwaArchiwum, tabPlikow[i], NULL);
			else if(n_spacji == 5)
			execlp("7z", "7z", "a", tabOdebrParam[0], tabOdebrParam[1], tabOdebrParam[2], tabOdebrParam[3], tabOdebrParam[4], nazwaArchiwum, tabPlikow[i], NULL);
			else if(n_spacji == 6)
			execlp("7z", "7z", "a", tabOdebrParam[0], tabOdebrParam[1], tabOdebrParam[2], tabOdebrParam[3], tabOdebrParam[4], tabOdebrParam[5], nazwaArchiwum, tabPlikow[i], NULL);
		}
		else
		{ 
			/* by nie nastapily problemy podczas dodawania kolejnych plikow do archiwum proces obslugujacy klienta czeka na zakonczenie obecnego procesu dodawania pliku zanim rozpocznie dodawanie kolejnego */
			waitpid(child_pid,&status,0);
			continue;
		}
	}
	printf("Potomny: Zakonczono tworzenie archiwum %s.\n", nazwaArchiwum);
	
	/* odeslanie gotowego archiwum */
	wyslijPlik(gniazdoK, nazwaArchiwum);

	close(gniazdoK);

	/* czyszczenie plikow i pamieci */

	for(i = 0; i<ile; i++)
	{
		if((child_pid=fork()) == 0)
		{
			execlp("rm","rm", tabPlikow[i], NULL);
		}
		else
		{ 
			waitpid(child_pid,&status,0);
		}
	}

	if((child_pid=fork()) == 0)
	{
		execlp("rm","rm", nazwaArchiwum, NULL);
	}
	else
	{ 
		waitpid(child_pid,&status,0);
	}
	
	for(i = 0; i<ile; i++)
	{
		free(tabPlikow[i]);	
	}
	free(tabPlikow);

	free(tabOdebrParam);
}

int main(void)
{
int gniazdoNasluch,gniazdoKlient;
struct sockaddr_in serv;
socklen_t servdl = sizeof(struct sockaddr_in);

gniazdoNasluch = socket(PF_INET, SOCK_STREAM, 0);
serv.sin_family = AF_INET;
serv.sin_port = htons(5656);
serv.sin_addr.s_addr = INADDR_ANY;
memset(serv.sin_zero, 0, sizeof(serv.sin_zero));

if( bind(gniazdoNasluch,(struct sockaddr*) &serv, sizeof(serv)) < 0 )
{ 
	printf("Bind nie powiodl sie!\n"); 
	return 1; 
}

if (listen(gniazdoNasluch, 10) < 0) 
 {
	printf("Listen nie powiodl sie.\n");
	return 1; 
 }

 
 while(1)
 {
	gniazdoKlient = accept(gniazdoNasluch, (struct sockaddr*) &serv, &servdl);
	if (gniazdoKlient < 0)
	{
		printf("Glowny: accept zwrocil blad\n");
		continue;
	}
	
	printf("Glowny: polaczenie od %s:%u\n", 
	inet_ntoa(serv.sin_addr),
	ntohs(serv.sin_port)
	);
	
	if(fork() == 0)
	{
		printf("Potomny: rozpoczynam obsluge klienta\n");
		obslugaKlienta(gniazdoKlient);
	}
    else
    {
		/*proces macierzysty kontynuuje prace*/
        continue;
    }
 }
return 0;
}

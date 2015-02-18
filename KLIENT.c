#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

void odbiorPliku(int gniazdoK)
{
	char filename[512], bufor[1024];
	long rozmiar,odebrano,odebrano_razem;
	FILE *plik;
	char potw;

	memset(filename,0,512);
	odebrano_razem = 0;

	/* odbior nazwy pliku */
	if( recv(gniazdoK, filename, sizeof(filename), 0) != sizeof(filename) ) { printf("Potomny: blad podczas odbierania odbieranego pliku."); return; }
	/*odbior rozmiaru pliku */
	if( recv(gniazdoK, &rozmiar, sizeof(long), 0) != sizeof(long) ) { printf("Potomny: blad podczas odbierania rozmiaru pliku."); return; }
	rozmiar = ntohl(rozmiar);
	printf("Potomny: Nastapi odebranie pliku o nazwie %s, o rozmiarze %ld\n",filename, rozmiar);
	
	printf("Potomny: Rozpoczeto odbieranie bajtow pliku...\n");
	plik = fopen(filename,"wb");
	while(odebrano_razem < rozmiar)
	{
		memset(bufor,0,1024);
		odebrano = recv(gniazdoK, bufor, sizeof(bufor), 0);
		if(odebrano < 0) { break; }
		odebrano_razem += odebrano;
		fwrite(bufor, 1, odebrano, plik);
	}
	fclose(plik);

    if (odebrano_razem != rozmiar)
	{
        printf("*** BLAD W ODBIORZE PLIKU %s ***\n",filename);
		potw = 0;
		send(gniazdoK,&potw,sizeof(potw),0);
	}
    else
	{
        printf("*** PLIK %s ODEBRANY POPRAWNIE ***\n",filename);	
		potw = 1;
		send(gniazdoK,&potw,sizeof(potw),0);
	}
}

void wyslijPlik(int sdsocket, char *nazwa)
{
	char filename[512],bufor[1024];

	strcpy(filename,nazwa);

	send(sdsocket,filename, sizeof(filename),0);

	struct stat fileinfo;
    FILE* plik;

    if (stat(nazwa, &fileinfo) < 0)
    {
        printf("Potomny: nie moge pobrac informacji o pliku\n");
        return;
    }
	printf("Rozmiar pliku: %lld ", fileinfo.st_size);
	long dlpliku,wyslano,wyslano_razem,przeczytano;
	dlpliku = htonl((long) fileinfo.st_size);

	send(sdsocket, &dlpliku, sizeof(long), 0);
	
	dlpliku = fileinfo.st_size;
	wyslano_razem=0;
	plik = fopen(nazwa,"rb");

    while (wyslano_razem < dlpliku)
    {
		memset(bufor,0,1024);
        przeczytano = fread(bufor, 1, 1024, plik);
        wyslano = send(sdsocket, bufor, przeczytano, 0);
        if (przeczytano != wyslano)
            break;
        wyslano_razem += wyslano;
    }
	fclose(plik);
	char potw;
	recv(sdsocket,&potw,sizeof(char),0);
	if (potw == 1)
        printf("Potomny: plik wyslany poprawnie\n");
    else
        printf("Potomny: blad przy wysylaniu pliku\n");

}

int main(int argc, char **argv) {
struct sockaddr_in endpoint;
int sdsocket;
short liczba;
struct hostent *he;
  if (argc<3) {
        printf("podaj nazwe hosta i numer portu jako parametry\n");
        return 1;
    }
    he = gethostbyname(argv[1]);
    if (he == NULL) {
        printf("Nieznany host: %s\n",argv[1]);
        return 0;
    }
	
	endpoint.sin_family = AF_INET;
    endpoint.sin_port = htons(atoi(argv[2]));
    endpoint.sin_addr = *(struct in_addr*) he->h_addr_list[0];
	liczba =(short)1; /* LICZBA PLIKOW DO WYSLANIA */
	liczba=htons(liczba);
	        if ((sdsocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("socket() nie powiodl sie\n");
            return 1;
        }
		if (connect(sdsocket,(struct sockaddr*) &endpoint, sizeof(struct sockaddr_in)) < 0) {
            printf("connect() nie powiodl sie\n");
            return 0;
        }
		send(sdsocket, &liczba, sizeof(short), 0);
		

	wyslijPlik(sdsocket,"opis_projektu.txt");

	char parametry[256];
	memset(parametry, 0, 256);
	sprintf(parametry,"-t7z -phaslo");
	send(sdsocket, parametry, sizeof(parametry), 0);

	odbiorPliku(sdsocket);


		close(sdsocket);
	
}

/********************************************************
* Projet de C - M1 STRI                                 *
* TULEQUE Mika�l - WATRE Tony - PRIETO Florian          *
*														*
* Fichier : serveur.c									*
*														*
* Descriptif :											*
*														*
* Fichier contenant le corps de toutes les proc�dures et*
* fonctions du serveur.									*
* 														*
* 														*
*														*
********************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <dirent.h>

#include <errno.h>

#include "serveur.h"

#define TRUE 1
#define FALSE 0

#ifdef WIN32
#define perror(x) printf("%s : code d'erreur : %d\n", (x), WSAGetLastError())
#define close closesocket
#define socklen_t int
#endif

/* Variables cachees */

/* le socket d'ecoute */
int socketEcoute;
/* longueur de l'adresse */
socklen_t longeurAdr;

/* Initialisation.
 * Creation du serveur en pr�cisant le service ou num�ro de port.
 * renvoie 1 si �a c'est bien pass� 0 sinon
 */
int InitialisationAvecService(char *service) {
	int n;
	const int on = 1;
	struct addrinfo	hints, *res, *ressave;

	#ifdef WIN32
	WSADATA	wsaData;
	if (WSAStartup(0x202,&wsaData) == SOCKET_ERROR)
	{
		printf("WSAStartup() n'a pas fonctionne, erreur : %d\n", WSAGetLastError()) ;
		WSACleanup();
		exit(1);
	}
	memset(&hints, 0, sizeof(struct addrinfo));
    #else
	bzero(&hints, sizeof(struct addrinfo));
	#endif

	/* On initialise les variables en mode connect� */
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	/* On r�cup�re les informations avec getaddrinfo() */
	if ( (n = getaddrinfo(NULL, service, &hints, &res)) != 0)  {
			/* En cas d'�chec on informe l'utilisateur */
     		printf("Initialisation, erreur de getaddrinfo : %s", gai_strerror(n));
     		return 0;
	}
	/* On r�alise ne sauvegarde des informations */
	ressave = res;

	/* On cr�er le socket d'�coute pour les clients */
	do {
		socketEcoute = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (socketEcoute < 0)
			continue;		/* error, try next one */

		setsockopt(socketEcoute, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
#ifdef BSD
		setsockopt(socketEcoute, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
#endif
		/* On associe le socket au serveur */
		if (bind(socketEcoute, res->ai_addr, res->ai_addrlen) == 0)
			break;			/* success */

		close(socketEcoute);	/* bind error, close and try next one */
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL) {
     		perror("Initialisation, erreur de bind.");
     		return 0;
	}

	/* conserve la longueur de l'addresse */
	longeurAdr = res->ai_addrlen;

	/* On lib�re la sauvegarde */
	freeaddrinfo(ressave);
	/* On cr�er une file d'attente de maximum 5 clients */
	listen(socketEcoute, 5);
	/* Success */
	return 1;
}

/* Attends qu'un client se connecte, quand un client se connecte on lance un processus fils pour le traitement de sa requ�te */
Client * AttenteClient() {
	struct sockaddr *clientAddr;
	char machine[NI_MAXHOST]; 
	Client *client; /* Client qui se connecte /

	/* On cr�er la structure client et on lui alloue de la m�moire */
	client = (Client*) malloc(sizeof(Client));
	clientAddr = (struct sockaddr*) malloc(longeurAdr);

	/* On connecte le client au socket*/
	client->socketService = accept(socketEcoute, clientAddr, &longeurAdr);

	/* On v�rifie que la connexion du client au socket s'est bien pass�e */
	if (client->socketService == -1) {
		perror("AttenteClient, erreur de accept.");
		free(clientAddr);
		free(client);
		return 0;
	}

	/* On r�cup�re les informations sur le client qui se connecte */
	if(getnameinfo(clientAddr, longeurAdr, machine, NI_MAXHOST, NULL, 0, 0) == 0) {
		printf("Client sur la machine d'adresse %s connecte.\n", machine);
	} else {
		printf("Client anonyme connecte.\n");
	}

	/* On lib�re la m�moire allou�e � l'adresse client */
	free(clientAddr);

	/*
	 * Reinit buffer
	 */
	client->debutTampon = 0;
	client->finTampon = 0;	
	return client;
}

/* Recoit un message envoye par le serveur.
 */
char *Reception(Client *client) {
	char message[LONGUEUR_TAMPON]; /* Message re�u du client */
	unsigned short int messageLength; /* Variable contenant la taille du message */
	
	/* On remplit message de z�ro pour connaitre la fin */
	memset(message,0,sizeof message);

	/* On bucle en attendant de recevoir le message du client */
	while(recv(client->socketService,message,sizeof message,0) > 0){
		messageLength = strlen(message); /* On r�cup�re la longueur du message */
		if(messageLength > 0){
			/* Si le message contient au moins 1 caract�re on le retourne */
			return message;
		}
	}
}

/* Envoie un message au client.
 */
int Emission(char *message, Client *client) {
	int taille; /* Longueur du message */

	/* On v�rifie que le message se termine bien par \n */
	if(strstr(message, "\n") == NULL) {
		/* Si le message ne finit pas par \n, on le rajoute � la fin */
		strcat(message,"\n");
	}

	/* On r�cup�re la taille du message */
	taille = strlen(message);

	/* On envoi le message au client */
	if (send(client->socketService, message, taille,0) == -1) {
        perror("Emission, probleme lors du send.\n");
        return 0;
	}

	return 1;
}


/* Ferme la connexion avec le client.
 */
void TerminaisonClient(Client *client) {
	close(client->socketService);
	free(client);
}

/* Arrete le serveur.
 */
void Terminaison() {
	close(socketEcoute);
}

/* Met tous les caract�res d'une cha�ne en majuscule */
char * putMajuscule(char *ch){
    int i; /* indice de parcour de la cha�ne */
    for(i=0;i<strlen(ch)-1;i++){
        /* pour chaque caract�res de ch on le remplace par la minuscule correspondante */
        ch[i] = toupper(ch[i]);
    }
    return ch;
}

/* Liste le r�pertoire pass� en param�tre, les diff�rents �l�ments seront s�par�s par des # */
char * listeDir(char *repCourrant){


	DIR* dir = NULL; /* Dossier ouvert */
	struct dirent* fic = NULL; /* fichier s�lectionn� */
	char * nomFic = NULL;
	char * listeFic = NULL; /* Liste des des fichiers pr�sents dans le dossier */

	/* On ouvre le r�pertoire pass� en param�tre */
	dir = opendir(repCourrant);


	/* On v�rifie que le dossier a bien �t� ouvert */
	if(dir == NULL){
		perror("");
		return NULL;
	}


	/* On va lister l'ensemble du r�pertoire */
	while((fic = readdir(dir)) != NULL){
		nomFic = NULL;
		nomFic = fic->d_name;
		if(strchr(nomFic,'.') == NULL){
			printf("Dossier = %s\n",nomFic);
		}else{
			if(strcmp(nomFic,".") != 0){
				if(strcmp(nomFic,"..") != 0){
					printf("Fichier = %s\n",nomFic);
				}
			}
		}
	}

	/* On renvoi la liste des fichiers */
	return listeFic;
}
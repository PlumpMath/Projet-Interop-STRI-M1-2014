
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <dirent.h>
#ifndef WIN32
    #include <sys/types.h>
#endif
#include "client.h"

#define TRUE 1
#define FALSE 0
#define LONGUEUR_TAMPON 4096

/* le socket client */
int socketClient;

/* Initialisation.
 * Connexion au serveur sur la machine donnee et au service donne.
 * Utilisez localhost pour un fonctionnement local.
 */
int InitialisationAvecService(char *machine, char *service) {
	int n; /* Variable permettant de tester le retour de la fonction getaddrinfo() */
	struct addrinfo	hints, *res, *ressave; /*Variables permettant la cr�ation et l'initialisation du socket */

/* On met les valeurs du mode connect� dans les variables */
	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	/* On r�cup�re les informations avec la fonction getaddrinfo() */
	if ( (n = getaddrinfo(machine, service, &hints, &res)) != 0)  {
			/* En casd'erreur on informe l'utilisateur */
     		fprintf(stderr, "Initialisation, erreur de getaddrinfo : %s", gai_strerror(n));
     		return 0;
	}

	/* On fait une sauvegarde des informations */
	ressave = res;

	/* On initialise le socket client */
	do {
		socketClient = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (socketClient < 0)
			continue;	/* ignore this one */

		/* On r�alise une demande de connexion sur le socket */
		if (connect(socketClient, res->ai_addr, res->ai_addrlen) == 0)
			break;		/* success */

		close(socketClient);	/* ignore this one */
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL) {
     		perror("ERREUR : probl�me de connexion au serveur\n");
     		return 0;
	}

	freeaddrinfo(ressave); /* Si tout est ok on lib�re l variable de sauvegarde */

	/* Success */
	return 1;
}

/* Recoit un message envoye par le serveur.
 */
char *Reception() {
	char message[LONGUEUR_TAMPON]; /* variale contenant le message re�u */
	unsigned short int messageLength; /* Variable contenant la taille du message */
	
	/* On met le tampon � z�ro */
	memset(message,0,sizeof message);

	/* On r�cup�re le message du serveur */
	if(recv(socketClient,message,sizeof message,0) == -1){
		/* Si on r�cup�re rien on retourne null */
		return NULL;
	}

	/* On retourne le message que on a re�u du serveur */
	return message;
}

/* Envoie un message au serveur.
 */
int Emission(char *message) {
	/* On regarde si le message finit bien par \n */
	if(strstr(message, "\n") == NULL) {
		/* Si le message ne finit pas par \n, on le rajoute � la fin */
		strcat(message,"\n");
	}
	/* On r�cup�re la taille du message */
	int taille = strlen(message);
	/* On envoi le message au serveur */
	if (send(socketClient, message, taille,0) == -1) {
        perror("Emission, probleme lors du send\n");
        return 0;
	}
	/* Success */
	return 1;
}


/* Ferme la connexion.
 */
void Terminaison() {
	close(socketClient);
}

/* R�cup�re la liste des fichiers dans le r�pertoire courrant du serveur */
void listeFichiers(){

}
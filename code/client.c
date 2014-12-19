
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

/* 
Connexion au serveur FTP.
retourne 1 si la connexion est OK et 0 sinon
*/
int connecterUtilisateur(){

	char *message = NULL; /* Variable qui va contenir les messages du serveur */
	char utilisateur[50]; /* Nom d'utilisateur avec lequel on veut se connecter */
	int erreur = 0; /* variable qui permet de tester la pr�sence d'une erreur */
	char * requete; /* Requete que l'on va envoyer au serveur */

	/* On alloue de la m�moire pour la variable requete */
	requete = (char*) malloc(60);

	/* On r�cup�re le message du serveur */
	message = Reception();

	/* On affiche ce message */
	printf("%s",message);

	/* On va lire le nom d'utilisateur au clavier */
	fflush(stdin); /* On vide le tampon */
	fgets(utilisateur,50,stdin);

	/* On prepare la requete pour le serveur */
	strcat(requete,"USER ");
	strcat(requete,utilisateur);

	/* On envoi la requete au serveur */
	Emission(requete);

	/* On r�cup�re la reponse du serveur */
	message = NULL;
	message = Reception();

	/* On affiche cette r�ponse */
	printf("%s",message);

	/* On analyse la r�ponse du serveur pour voir la connxion est OK , il faut un message 230 */
	if(strstr(message,"230") != NULL){
		/* La r�ponse du serveur est bien de type 230 */
		return 1;
	}else{
		return 0;
	}
}

/* Envoi un fichier pr�sent dans le dossier courrant sur le serveur */
void envoyerFichier(char *nomFichier){
	FILE * fichier = NULL; /* Fichier que l'on veut envoyer */
	char *contenuFichier; /* Contenu du fichier que l'on veut envoyer */
	char *reponseServeur; /* R�ponse du serveur */
	char requete[100]; /* requete qu'on envoi au serveur */

	/* Ouverture du fichier en mode lecture */
	fichier = fopen(nomFichier,"rb");
	/* On teste l'ouverture du fichier */
	if(fichier == NULL){
		/* probl�me ouverture */
		printf("ERREUR : ouverture du fichier �chou�e\n");
	}else{
		/* on va maintenant lire le contenu du fichier */
		if(fread(contenuFichier,sizeof(FILE),1,fichier)<1){
			/* Erreur lecture fichier */
			printf("ERREUR : lecture du fichier �chou�e\n");
		}else{
			/* On teste que contenu fichier est non null */
			if(strcmp(contenuFichier,NULL) == 0){
				/* Contenu fichier null */
				printf("ERREUR : contenu du fichier null\n");
			}else{
				/* on pr�pare la requete pour le serveur */
				sprintf(requete,"STOR %s\n",nomFichier);
				/* On envoi la requete au serveur */
				Emission(requete);
				/* On affiche la r�ponse du serveur */
				reponseServeur = Reception();
				printf("%s",reponseServeur);
				/* Si la r�ponse est 150 - * on envoi le contenu */
				if(strstr(reponseServeur,"150") != NULL){
					/* Le serveur accepte la demande, on envoi le contenu */
					Emission(contenuFichier);
					/* On r�cup�re la reponse du serveur */
					reponseServeur = NULL;
					reponseServeur = Reception();
					printf("%s",reponseServeur);
				}
			}
		}
		/* On ferme le fichier */
		fclose(fichier);
	}
}
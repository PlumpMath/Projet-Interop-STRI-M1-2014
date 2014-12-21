
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
	struct addrinfo	hints, *res, *ressave; /*Variables permettant la création et l'initialisation du socket */

/* On met les valeurs du mode connecté dans les variables */
	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	/* On récupère les informations avec la fonction getaddrinfo() */
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

		/* On réalise une demande de connexion sur le socket */
		if (connect(socketClient, res->ai_addr, res->ai_addrlen) == 0)
			break;		/* success */

		close(socketClient);	/* ignore this one */
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL) {
     		perror("ERREUR : problème de connexion au serveur\n");
     		return 0;
	}

	freeaddrinfo(ressave); /* Si tout est ok on libère l variable de sauvegarde */

	/* Success */
	return 1;
}

/* Recoit un message envoye par le serveur.
 */
char *Reception() {
	char message[LONGUEUR_TAMPON]; /* variale contenant le message reçu */
	unsigned short int messageLength; /* Variable contenant la taille du message */
	
	/* On met le tampon à zéro */
	memset(message,0,sizeof message);

	/* On récupère le message du serveur */
	if(recv(socketClient,message,sizeof message,0) == -1){
		/* Si on récupère rien on retourne null */
		return NULL;
	}

	/* On retourne le message que l'on a reçu du serveur */
	return message;
}

/* Envoie un message au serveur.
 */
int Emission(char *message) {
	/* On regarde si le message finit bien par \n */
	if(strstr(message, "\n") == NULL) {
		/* Si le message ne finit pas par \n, on le rajoute à la fin */
		strcat(message,"\n");
	}
	/* On récupère la taille du message */
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

/* Récupère la liste des fichiers dans le répertoire courrant du serveur */
void listeFichiers(){

}

/* 
Connexion au serveur FTP.
retourne 1 si la connexion est OK et 0 sinon
*/
int connecterUtilisateur(){

	char *message = NULL; /* Variable qui va contenir les messages du serveur */
	char utilisateur[50]; /* Nom d'utilisateur avec lequel on veut se connecter */
	int erreur = 0; /* variable qui permet de tester la présence d'une erreur */
	char * requete; /* Requete que l'on va envoyer au serveur */

	/* On alloue de la mémoire pour la variable requete */
	requete = (char*) malloc(60);

	/* On récupère le message du serveur */
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

	/* On récupère la reponse du serveur */
	message = NULL;
	message = Reception();

	/* On affiche cette réponse */
	printf("%s",message);

	/* On analyse la réponse du serveur pour voir la connxion est OK , il faut un message 230 */
	if(strstr(message,"230") != NULL){
		/* La réponse du serveur est bien de type 230 */
		return 1;
	}else{
		return 0;
	}
}

/* Envoi un fichier présent dans le dossier courrant sur le serveur */
void envoyerFichier(char *nomFichier){
	FILE * fichier = NULL; /* Fichier que l'on veut envoyer */
	char *contenuFichier; /* Contenu du fichier que l'on veut envoyer */
	char *reponseServeur; /* Réponse du serveur */
	char requete[100]; /* requete qu'on envoi au serveur */
	long taille; /* taille du fichier */

	/* On supprime le \n à la fin du nomFichier */
	nomFichier[strlen(nomFichier)-1] = NULL;

	/* Ouverture du fichier en mode lecture */
	fichier = fopen(nomFichier,"rb");
	/* On teste l'ouverture du fichier */
	if(fichier == NULL){
		/* problème ouverture */
		printf("ERREUR : ouverture du fichier échouée\n");
	}else{
		/* On récupère la taille du fichier */
		fseek (fichier , 0 , SEEK_END);
  		taille = ftell (fichier);
  		rewind (fichier);

  		/* On alloue de la mémoire pour le contenu du fichier */
  		contenuFichier = malloc(taille);

		/* on va maintenant lire le contenu du fichier */
		if(fread(contenuFichier,taille*sizeof(char),1,fichier)<1){
			/* Erreur lecture fichier */
			/* On ferme le fichier */
			fclose(fichier);
			printf("ERREUR : lecture du fichier échouée\n");
		}else{
			/* On ferme le fichier */
			fclose(fichier);
			/* On teste que contenu fichier est non null */
			if(contenuFichier == NULL){
				/* Contenu fichier null */
				printf("ERREUR : contenu du fichier null\n");
			}else{
				/* on prépare la requete pour le serveur */
				sprintf(requete,"STOR %s\n",nomFichier);
				/* On envoi la requete au serveur */
				Emission(requete);
				/* On affiche la réponse du serveur */
				reponseServeur = Reception();
				printf("%s",reponseServeur);
				/* Si la réponse est 150 - * on envoi le contenu */
				if(strstr(reponseServeur,"150") != NULL){
					/* Le serveur accepte la demande, on envoi le contenu */
					Emission(contenuFichier);
					/* On récupère la reponse du serveur */
					reponseServeur = Reception();
					printf("%s",reponseServeur);	
				}
			}
		}
	}
}

/* Télécharge un fichier depuis le serveur */
void telechargerFichier(char *nomFichier){
	FILE * fichier = NULL; /* Fichier que l'on veut créer */
	char *contenuFichier; /* Contenu du fichier que l'on veut créer */
	char *reponseServeur; /* Réponse du serveur */
	char requete[100]; /* requete qu'on envoi au serveur */
	long taille; /* taille du fichier */

	/* On supprime le \n à la fin du nomFichier */
	nomFichier[strlen(nomFichier)-1] = NULL;

	/* On vérifie que le nom du fichier est non null ou vide */
	if(nomFichier == NULL || strcmp(nomFichier,"") == 0){
		/* Fichier null ou vide */
		printf("ERREUR : le nom du fichier est vide\n");
	}else{
		/* On prépare la requete pour le serveur */
		sprintf(requete,"RETR %s\n",nomFichier);
		/* On envoi la requete */
		Emission(requete);
		/* On affiche la réponse du serveur */
		reponseServeur = Reception();
		printf("%s",reponseServeur);
		/* Si la réponse est 150 - * on récupère le contenu le contenu */
		if(strstr(reponseServeur,"150") != NULL){
			/* On récupère le contenu du fichier */
			contenuFichier = Reception();
			/* On teste que le contenu est non null */
			if(contenuFichier == NULL){
				/* Erreur avec le transfert du contenu */
				printf("ERREUR : Contenu du fichier NULL\n");
			}else{
				/* On récupère le message du serveur et on l'affiche */
				/* On va maintenant créer le fichier en local */
				/* Ouverture du fichier en mode lecture */
				fichier = fopen(nomFichier,"wb");
				/* On teste l'ouverture du fichier */
				if(fichier == NULL){
					/* problème ouverture */
					printf("ERREUR : ouverture du fichier échouée\n");
				}else{
					/* On écrit le contenu du fichier téléchargé dans le fichier local */
					if(fwrite(contenuFichier,strlen(contenuFichier),1,fichier) < 1){
				    	/* Erreur écriture du fichier */
				    	/* On ferme le fichier */
						fclose(fichier);
				        printf("ERREUR : ecriture du contenu du fichier échouée\n");
				        /* On informe le serveur que le fichier est bien créé */
				    	Emission("KO\n");
				    }else{
				    	/* On ferme le fichier */
				    	fclose(fichier);
				    	/* On informe le serveur que le fichier est bien créé */
				    	Emission("OK\n");
				    	/* On récupère la réponse du serveur et on l'affiche*/
				    	reponseServeur = Reception();
				    	printf("%s",reponseServeur);
				    }
				}
			}
		}
	}
}


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
 * Utiliser localhost pour un fonctionnement local.
 */
int InitialisationAvecService(char *machine, char *service) {
	int n; /* Variable permettant de tester le retour de la fonction getaddrinfo() */
	struct addrinfo	hints, *res, *ressave; /*Variables permettant la creation et l'initialisation du socket */

/* On met les valeurs du mode connecte dans les variables */
	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	/* On recupere les informations avec la fonction getaddrinfo() */
	if ( (n = getaddrinfo(machine, service, &hints, &res)) != 0)  {
			/* En cas d'erreur, on informe l'utilisateur */
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

		/* On realise une demande de connexion sur le socket */
		if (connect(socketClient, res->ai_addr, res->ai_addrlen) == 0)
			break;		/* success */

		close(socketClient);	/* ignore this one */
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL) {
     		perror("ERREUR : probleme de connexion au serveur\n");
     		return 0;
	}

	freeaddrinfo(ressave); /* Si tout est ok on libere la variable de sauvegarde */

	/* Success */
	return 1;
}

/* Recoit un message envoye par le serveur.
 */
char *Reception() {
	char message[LONGUEUR_TAMPON]; /* variable contenant le message reçu */
	unsigned short int messageLength; /* Variable contenant la taille du message */
	
	/* On met le tampon a zero */
	memset(message,0,sizeof message);

	/* On recupere le message du serveur */
	if(recv(socketClient,message,sizeof message,0) == -1){
		/* Si on recupere rien on retourne null */
		return NULL;
	}

	/* On retourne le message que l'on a reçu du serveur */
	return message;
}

/* Envoi un message au serveur.
 */
int Emission(char *message) {
	/* On regarde si le message finit bien par \n */
	if(strstr(message, "\n") == NULL) {
		/* Si le message ne finit pas par \n, on le rajoute a la fin */
		strcat(message,"\n");
	}
	/* On recupere la taille du message */
	int taille = strlen(message);
	/* On envoie le message au serveur */
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

/* Recupere la liste des fichiers dans le repertoire courant du serveur */
void listeFichiers(){

}

/* 
Connexion au serveur FTP.
retourne 1 si la connexion est OK et 0 sinon
*/
int connecterUtilisateur(){

	char *message = NULL; /* Variable qui va contenir les messages du serveur */
	char utilisateur[50]; /* Nom d'utilisateur avec lequel on veut se connecte */
	int erreur = 0; /* variable qui permet de tester la presence d'une erreur */
	char * requete; /* Requete que l'on va envoyer au serveur */

	/* On alloue de la mémoire pour la variable requete */
	requete = (char*) malloc(60);

	/* On recupere le message du serveur */
	message = Reception();

	/* On affiche ce message */
	printf("%s",message);

	/* On va lire le nom d'utilisateur au clavier */
	fflush(stdin); /* On vide le tampon */
	fgets(utilisateur,50,stdin);

	/* On prepare la requete pour le serveur */
	strcat(requete,"USER ");
	strcat(requete,utilisateur);

	/* On envoie la requete au serveur */
	Emission(requete);

	/* On recupere la reponse du serveur */
	message = NULL;
	message = Reception();

	/* On affiche cette reponse */
	printf("%s",message);

	/* On analyse la reponse du serveur pour voir la connexion est OK , il faut un message 230 */
	if(strstr(message,"230") != NULL){
		/* La reponse du serveur est bien de type 230 */
		return 1;
	}else{
		return 0;
	}
}

/* Envoi un fichier present dans le dossier courant sur le serveur */
void envoyerFichier(char *nomFichier){
	FILE * fichier = NULL; /* Fichier que l'on veut envoyer */
	char *contenuFichier; /* Contenu du fichier que l'on veut envoyer */
	char *reponseServeur; /* Reponse du serveur */
	char requete[100]; /* requete qu'on envoie au serveur */
	long taille; /* taille du fichier */

	/* On supprime le \n a la fin du nomFichier */
	nomFichier[strlen(nomFichier)-1] = NULL;

	/* Ouverture du fichier en mode lecture */
	fichier = fopen(nomFichier,"rb");
	/* On teste l'ouverture du fichier */
	if(fichier == NULL){
		/* probleme ouverture */
		printf("ERREUR : ouverture du fichier echouee\n");
	}else{
		/* On recupere la taille du fichier */
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
			printf("ERREUR : lecture du fichier echouee\n");
		}else{
			/* On ferme le fichier */
			fclose(fichier);
			/* On teste que contenu fichier est non null */
			if(contenuFichier == NULL){
				/* Contenu fichier null */
				printf("ERREUR : contenu du fichier null\n");
			}else{
				/* on prepare la requete pour le serveur */
				sprintf(requete,"STOR %s\n",nomFichier);
				/* On envoie la requete au serveur */
				Emission(requete);
				/* On affiche la reponse du serveur */
				reponseServeur = Reception();
				printf("%s",reponseServeur);
				/* Si la reponse est 150 - * on envoie le contenu */
				if(strstr(reponseServeur,"150") != NULL){
					/* Le serveur accepte la demande, on envoie le contenu */
					Emission(contenuFichier);
					/* On recupere la reponse du serveur */
					reponseServeur = Reception();
					printf("%s",reponseServeur);	
				}
			}
		}
	}
}

/* Telecharge un fichier depuis le serveur */
void telechargerFichier(char *nomFichier){
	FILE * fichier = NULL; /* Fichier que l'on veut creer */
	char *contenuFichier; /* Contenu du fichier que l'on veut creer */
	char *reponseServeur; /* Reponse du serveur */
	char requete[100]; /* requete qu'on envoie au serveur */
	long taille; /* taille du fichier */

	/* On supprime le \n a la fin du nomFichier */
	nomFichier[strlen(nomFichier)-1] = NULL;

	/* On verifie que le nom du fichier est non null ou vide */
	if(nomFichier == NULL || strcmp(nomFichier,"") == 0){
		/* Fichier null ou vide */
		printf("ERREUR : le nom du fichier est vide\n");
	}else{
		/* On prepare la requete pour le serveur */
		sprintf(requete,"RETR %s\n",nomFichier);
		/* On envoie la requete */
		Emission(requete);
		/* On affiche la reponse du serveur */
		reponseServeur = Reception();
		printf("%s",reponseServeur);
		/* Si la reponse est 150 - * on recupere le contenu le contenu */
		if(strstr(reponseServeur,"150") != NULL){
			/* On recupere le contenu du fichier */
			contenuFichier = Reception();
			/* On teste que le contenu est non null */
			if(contenuFichier == NULL){
				/* Erreur avec le transfert du contenu */
				printf("ERREUR : Contenu du fichier NULL\n");
			}else{
				/* On recupere le message du serveur et on l'affiche */
				/* On va maintenant creer le fichier en local */
				/* Ouverture du fichier en mode lecture */
				fichier = fopen(nomFichier,"wb");
				/* On teste l'ouverture du fichier */
				if(fichier == NULL){
					/* probleme ouverture */
					printf("ERREUR : ouverture du fichier echouee\n");
				}else{
					/* On ecrit le contenu du fichier telecharge dans le fichier local */
					if(fwrite(contenuFichier,strlen(contenuFichier),1,fichier) < 1){
				    	/* Erreur ecriture du fichier */
				    	/* On ferme le fichier */
						fclose(fichier);
				        printf("ERREUR : ecriture du contenu du fichier echouee\n");
				        /* On informe le serveur que le fichier est bien cree */
				    	Emission("KO\n");
				    }else{
				    	/* On ferme le fichier */
				    	fclose(fichier);
				    	/* On informe le serveur que le fichier est bien cree */
				    	Emission("OK\n");
				    	/* On recupere la reponse du serveur et on l'affiche*/
				    	reponseServeur = Reception();
				    	printf("%s",reponseServeur);
				    }
				}
			}
		}
	}
}

/* Envoi au serveur une demande de changement du mode de telechargement des fichiers */
void changerMode(char mode){
	char requete[7]; /* Requete que l'on va envoyer au serveur */
	/* On prépare la requete */
	sprintf(requete,"MODE %c\n",mode);
	/* On envoi la requete au serveur */
	Emission(requete);
	/* On affiche la réponse du serveur */
	printf(Reception());
}

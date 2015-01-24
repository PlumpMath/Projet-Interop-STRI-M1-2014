
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
#include <pthread.h>
#ifndef WIN32
    #include <sys/types.h>
#endif
#include "client.h"

#define TRUE 1
#define FALSE 0
#define LONGUEUR_TAMPON 4096

/* Strucutre de donnees pour les thread de telechargement */
struct donneesThread{
	char *numPort;
	char *nomFichier;
};

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
	return strdup(message);
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

/* Recoit des donnees envoyees par le serveur.
 */
int ReceptionBinaire(char *donnees, size_t tailleMax) {
	int dejaRecu = 0;
	int retour = 0;
	/* si on n'est pas arrive au max
	 * on essaie de recevoir plus de donnees
	 */
	while(dejaRecu < tailleMax) {
		retour = recv(socketClient, donnees + dejaRecu, tailleMax - dejaRecu, 0);
		if(retour < 0) {
			perror("ReceptionBinaire, erreur de recv.");
			return -1;
		} else if(retour == 0) {
			fprintf(stderr, "ReceptionBinaire, le serveur a ferme la connexion.\n");
			return 0;
		} else {
			/*
			 * on a recu "retour" octets en plus
			 */
			dejaRecu += retour;
		}
	}
	return dejaRecu;
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

/* Telecharge un fichier depuis le serveur */
int telechargerFichierBloc(char *nomFichier){
	FILE * fichier = NULL; /* Fichier que l'on veut créer */
	char *reponseServeur; /* Réponse du serveur */
	char requete[100]; /* requête qu'on envoie au serveur */
	char bloc[65536]; /* Bloc de fichier reçu du serveur, taille max des données + 3 octets d'en-tête */
	char *donneesBloc; /* Données contenues dans le bloc */
	char descripteurBloc[3]; /* champ descripteur de l'en-tête*/
	char tailleBloc[4]; /* champ taille de l'en-tête */
	int i; /* indice de parcours */

	/* On supprime le \n à la fin de nomFichier */
	nomFichier[strlen(nomFichier)-1] = NULL;

	/* On vérifie si le nom du fichier est NULL ou vide */
	if(nomFichier == NULL || strcmp(nomFichier,"") == 0){
		/* Fichier null ou vide */
		printf("ERREUR : le nom du fichier est vide\n");
		return 0;
	}else{
		/* On prépare la requête pour le serveur */
		sprintf(requete,"RETR %s\n",nomFichier);
		/* On envoie la requete */
		Emission(requete);
		/* On affiche la reponse du serveur */
		reponseServeur = Reception();
		printf("%s",reponseServeur);
		/* Si la reponse est 150 - * on recupere le contenu */
		if(strstr(reponseServeur,"150") != NULL){
			/* On ouvre le fichier dans lequel on va écrire */
			fichier = fopen(nomFichier,"wb");
			/* On teste l'ouverture du fichier */
			if(fichier == NULL){
				/* probleme ouverture */
				printf("ERREUR : ouverture du fichier echouee\n");
				/* On informe le serveur du probleme */
				Emission("KO\n");
				/* On sort de la fonction en echec */
				return 0;
			}else{
				/* On va récupérer les blocs les uns après les autres jusqu'à recevoir un descripteur = 64 */
				do{
					/* On récupère l'entete du bloc */
					ReceptionBinaire(bloc,3);
					
					/* On regarde si le descripteur est 0 */
					if(bloc[0] == 0){
						/* On récupère les données */
						unsigned short taille;
						memcpy(&taille,bloc+1,2);
						taille = ntohs(taille);
						ReceptionBinaire(bloc,taille);
						/* On va ecrire les donnees dans le fichier */
						if(fwrite(bloc,taille,1,fichier) < 1){
							/* Erreur ecriture du fichier */
					        printf("ERREUR : ecriture du contenu du fichier echouee\n");
					        /* On informe le serveur que le fichier est bien cree */
					    	Emission("KO\n");
					    	/* On ferme le fichier */
					    	fclose(fichier);
					    	return 0;
						}else{
							/* On informe le serveur qu'on a bien reçu le bloc */
							Emission("OK\n");
						}
						/* On libere l'espace alloué aux données */
						//memset(donneesBloc,0,sizeof(donneesBloc));
						//free(donneesBloc);
					}
				}while(bloc[0] != 64);

				/* on ferme le fichier et on informe le serveur que c'est OK */
				fclose(fichier);
				Emission("OK\n");
				/* On affiche le message du serveur */
				printf("%s\n",Reception());
				return 1;
			}
		}
	}
}

/* Envoi au serveur une demande de changement du mode de telechargement des fichiers */
void changerMode(char mode){
	char requete[7]; /* Requete que l'on va envoyer au serveur */
	/* On prépare la requete */
	sprintf(requete,"MODE %c\n",mode);
	/* On envoie la requete au serveur */
	Emission(requete);
	/* On affiche la réponse du serveur */
	puts(Reception());
}

/* Permet de reprendre un téléchargement en cours en cas d'erreur */
int repriseTelechargement(char *nomFichier){
	FILE *fichier = NULL; /* fichier que l'on veut reprendre */
	long taille; /* taille actuelle du fichier */
	char *reponseServeur; /* Reponse du serveur */
	char requete[100]; /* requete qu'on envoie au serveur */
	char *bloc; /* Bloc de fichier reçu du serveur, taille max des données + 3 octets d'en-tête */
	char *donneesBloc; /* Donnees contenues dans le bloc */
	char descripteurBloc[3]; /* champ descripteur de l'en-tête*/
	char tailleBloc[4]; /* champ taille de l'en-tête */
	int i; /* indice de parcours */

	/* On supprime le \n a la fin du nomFichier */
	nomFichier[strlen(nomFichier)-1] = NULL;

	/* On verifie que le nom du fichier est non null ou vide */
	if(nomFichier == NULL || strcmp(nomFichier,"") == 0){
		/* Fichier null ou vide */
		printf("ERREUR : le nom du fichier est vide\n");
		return 0;
	}else{
		/* On ouvre le fichier en mode ajout */
		fichier = fopen(nomFichier,"ab");
		if(fichier == NULL){
			/* probleme ouverture */
			printf("ERREUR : ouverture du fichier echouee\n");
			/* On informe le serveur du probleme */
			Emission("KO\n");
			/* On sort de la fonction en echec */
			return 0;
		}else{
			/* On recupere la taille du fichier */
			fseek (fichier , 0 , SEEK_END);
			taille = ftell (fichier);
			rewind (fichier);

			/* On prépare la requete pour le serveur */
			sprintf(requete,"REST %ld\n",taille);
			/* On envoie la requête au serveur */
			Emission(requete);
			/* On récupère la reponse serveur et on regarde si elle contient 150 */
			reponseServeur = Reception();
			printf("%s",reponseServeur);
			if(strstr(reponseServeur,"150") != NULL){
				/* On va maintenant réaliser la même fonction que pour le mode bloc */
				/* On va récupérer les blocs les uns après les autres jusqu'à recevoir un descripteur = 64 */
				do{
					bloc = Reception();
					//printf("Bloc : %s",bloc);
					/* On va dans un premier temps extraire les différents champs d'en-tête du bloc */
					sprintf(descripteurBloc,"%c%c%c",bloc[0],bloc[1],bloc[2]);
					sprintf(tailleBloc,"%c%c%c%c",bloc[3],bloc[4],bloc[5],bloc[6]);
					/* On regarde si le descripteur est 0 */
					if(atoi(descripteurBloc) == 0 || atoi(descripteurBloc) == 16){
						if(atoi(descripteurBloc) == 16){
							printf("Debut de la reprise\n");
						}
						/* On récupère les données */
						donneesBloc = (char*) malloc(atoi(tailleBloc));
						for(i=7;i<strlen(bloc)-1;i++){
							donneesBloc[i-7] = bloc[i];
						}
						/* On va ecrire les donnees dans le fichier */
						if(fwrite(donneesBloc,atoi(tailleBloc),1,fichier) < 1){
							/* Erreur ecriture du fichier */
					        printf("ERREUR : ecriture du contenu du fichier echouee\n");
					        /* On informe le serveur que le fichier est bien cree */
					    	Emission("KO\n");
					    	/* On ferme le fichier */
					    	fclose(fichier);
					    	return 0;
						}else{
							/* On informe le serveur qu'on a bien reçu le bloc */
							Emission("OK\n");
						}
						/* On libere l'espace alloué aux données */
						memset(donneesBloc,0,sizeof(donneesBloc));
						free(donneesBloc);
					}
				}while(atoi(descripteurBloc) != 64);

				/* on ferme le fichier et on informe le serveur que c'est OK */
				fclose(fichier);
				Emission("OK\n");
				/* On affiche le message du serveur */
				printf("%s\n",Reception());
				return 1;
			}
		}

		
	}

	
}

void* telechargerFichierBlocThread(void* donnees){

}

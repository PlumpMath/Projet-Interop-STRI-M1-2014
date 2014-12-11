
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
Fonction permettant l'envoi � une machine distante du fichier dont le chemin est pass� en param�tre.
 */
void EnvoiFichier (char * nomFichier)
{
	/*
	envoyer(demande)
	lire(reponse)
	si reponse == ok alors
		envoyer(nom)
		lire(reponse)
		si reponse == ok alors
			envoyer(fichier)
			afficher(reponse)
		sinon
			afficher(erreur)
		fin
	sinon
		afficher(ereur)
	fin
	*/

	FILE * fichier = NULL; /* Fichier que l'on veut envoyer */
	char * contenuFichier; /* Contenu fu fichier en param�tre*/
	char * reponseServeur; /* reponse du serveur apr�s une �mission */

	/* On teste que le chemin pour le fichier n'est pas nul */
	if (nomFichier == NULL || strlen(nomFichier) < 1)
	{ 
		/* Si le chemin du fichier est null ou vide alors on envoi un message d'erreur */
		printf("ERREUR : Nom du fichier en param�tre null ou vide\n");
	}
	else
	{
		//Ouverture du fichier
		fichier = fopen(nomFichier, "rb");
		if(fichier == NULL)
		{
			/* probleme d'ouverture du fichier on sort en erreur */
			printf("ERREUR ; ouverture du fichier %s impossible\n",nomFichier);
		}
		else
		{
			/* On stocke le contenu du fichier dans la variable */
   			if(fread(contenuFichier,1 * sizeof(fichier),1,fichier)<1){
   				/* Probl�me lecture du fichier */
        		printf("ERREUR : lecture du fichier KO\n");
        		fclose(fichier);
    		}else{
    			/* On envoi le nom du fichier */
    			if(Emission(nomFichier) != 1){
    				/* Si erreur emission on sort en erreur */
    				printf("ERREUR : emission du nom du fichier KO\n");
    				fclose(fichier);
    			}else{
    				/* on lit la r�ponse du serveur */
    				reponseServeur = Reception();
    				/* On regarde si la r�ponse du serveur commence par "OK" */
    				char * testReponse = NULL; /* variable permettant de tester la r�ponse du serveur */
    				/* On ajoute les deux premiers caract�res de la r�ponse pour voir si elle commence par OK */
    				strcat(testReponse,reponseServeur[0]);
    				strcat(testReponse,reponseServeur[1]);
    				if(strcmp("OK",testReponse == 0)){
    					/* On a re�u OK on peut envoyer le contenu */
    					/* on envoi le fichier serveur */
		    			if(Emission(contenuFichier) == 1){
		    				/* Si c'est bon on ferme le fichier */
		    				fclose(fichier);
		    				/* On attend la r�ponse du serveur et on l'affiche */
		    				printf(Reception());
		    			}else{
		    				/* Erreur d'�mission */
		    				printf("ERREUR : probl�me d'�mission\n");
		    				fclose(fichier);
		    			}
    				}else{
    					/* Sinon on affiche le message d'erreur du serveur */
    					printf(reponseServeur);
    					fclose(fichier);
    				}
    			}
    			
    		}
		}
	}
	
}

/* 
Permet de cr�er un fichier de nom pass� en param�tre � partir du contenu pass� en param�tre.
*/
void ReceptionFichier(char * nomFichier)
{
	/*
	envoyer(demande)
	lire(reponse)
	si reponse = ok alors
		envoyer(nomFichier)
		lire(reponse)
		si reponse = ok alors
			lire(contenu)
			creerFichier(contenu)
		sinon
			afficher(reponse)
		fin
	sinon
		afficher(reponse)
	fin
	*/

	FILE * fichier = NULL; /* Fichier dans lequel on va �crire */
	char * contenuFichier; /* Contenu du fichier re�u du serveur */
	char * reponseServeur; /* reponse du serveur */

	/* On teste que le chemin pour le fichier n'est pas nul */
	if (nomFichier == NULL || strlen(nomFichier) < 1)
	{
		/* Si nomFichier null ou vide on sort en erreur */
		printf("ERREUR : Nom du fichier NULL ou vide \n");
	}else{
		/* Ouverture du fichier en mode �criture (�crase si d�j� existant) */
	    fichier = fopen(nomFichier,"wb");
	    if(fichier == NULL)
	    {
	    	/* Si probl�me d'ouverture du fichier on sort en erreur */
	        printf("ERREUR : ouverture du fichier %s KO\n",nomFichier);
	    }else{
	    	/* on envoi le nom du fichier que on veut t�l�charger au serveur */
	    	if(Emission(nomFichier) != 1){
    			/* Si erreur emission on sort en erreur */
    			printf("ERREUR : emission du nom du fichier KO\n");
    			fclose(fichier);
    		}else{
    			/* On attend la r�ponse du serveur */
    			reponseServeur = Reception();
    			/* On regarde si la r�ponse du serveur commence par "OK" */
    			if(strcmp("OK",reponseServeur[0]+reponseServeur[1]) == 0){
    				contenuFichier = Reception(); /* on r�cup�re le contenu du fichier depuis le serveur */
    				/* on ajoute le contenu dans le fichier */
				    if(fwrite(contenuFichier,1*sizeof(contenuFichier),1,fichier) < 1)
				    {
				    	/* Erreur cr�ation du fichier */
				        printf("ERREUR : ecriture fichier KO\n");
				    }else{
				    	printf("Telechargement du fichier %s --> OK\n",nomFichier); /* on informe du succes du t�l�chargement du fichier */
				    	/* on ferme le fichier */
				    	fclose(fichier);
				    }
    			}
    		}
	    }	
	}
}
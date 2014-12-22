#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "serveur.h"

#define TRUE 1
#define FALSE 0

int main(int argc, char *argv[]) {
	Client *client; /* Variable permettant de recuperer le client connecte sur le serveur */
	int termine; /* Variable qui permet de savoir si un client a termine ses traitements */
	char *message = NULL; /* Variable pour les requetes clients */
	char *commande = NULL; /* Commande FTP envoyee par un utilisateur */
	
	/* initialisation du serveur */
	if(InitialisationAvecService(argv[1]) == 1){
		/* Initialisation OK */
		printf("Initialisation du serveur -> OK \n");
	}else{
		/* Probleme d'initialisation */
		return -1;
	}

	
	char * test = listeDir("/home/florian/Dropbox/STRI/cours/interop/projet1/fichiersServeur");
	if(test == NULL){
		printf("test = NULL\n");
		return -1;
	}
	printf("%s\n",test);

	/* On boucle pour avoir un service continu */
	while(1){
		/* On recupere le client qui se connecte sur le serveur*/
		client = AttenteClient();
		/* On met la variable de fin des traitements a faux */
		termine = FALSE;
		do{
			/* On verifie que la variable message est bien positionnee sur NULL et sinon on la met a NULL */
			if(message != NULL){
				message = NULL;
			}
			/* On recupere la requete du client */
			message = Reception(client);
			/* On teste si on a recu quelque chose */
			if(message != NULL && strlen(message) > 0){

				/* On a bien recu la requete du client 
				 Requete sous forme commande#arg1#arg2... 
				 On recupere donc dans un premier temps la commande */

				 /* On regarde si commande est bien vide et sinon on le met a NULL */
				 if(commande != NULL){
				 	commande = NULL;
				 }

				 /* On recupere la commande dans la variable commande */
				 if(sscanf(message,"%s#",commande) > 0){
				 	/* On met la commande en majuscule pour eviter les problemes de casse */
				 	commande = putMajuscule(commande);
				 	printf("On a recu la commande : %s\n", commande);
				 }
			}

		}while(termine != TRUE);
	}
	/* On libere le socket */
	Terminaison();
	return 0;
}

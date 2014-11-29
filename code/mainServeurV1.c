/********************************************************
* Projet de C - M1 STRI                                 *
* TULEQUE Mikaël - WATRE Tony - PRIETO Florian          *
*														*
* Fichier : mainServeurV1.c								*
*														*
* Descriptif :											*
*														*
* Le serveur va créer un thread pour chaque client 	*
* qui se connecte, avec maximum 5 clients en même temps.*
* Le serveur proposera aux client la version minimaliste*
* du protocole FTP.										*
*														*
********************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "serveur.h"

#define TRUE 1
#define FALSE 0

int main(int argc, char *argv[]) {
	Client *client; /* Variable permettant de récupérer le client connecté sur le serveur */
	int termine; /* Variable qui permet de savoir si un client a terminé ses traitements */
	char *message = NULL; /* Variable pour les requêtes clients */
	char *commande = NULL; /* Commande FTP envoyer par un utilisateur */
	
	/* initialisation du serveur */
	if(InitialisationAvecService(argv[1]) == 1){
		/* Initialisation OK */
		printf("Initialisation du serveur -> OK \n");
	}else{
		/* Problème d'initialisation */
		return -1;
	}

	/* On boucle pour avoir un service continu */
	while(1){
		/* On récupère le client qui se connecte sur le serveur*/
		client = AttenteClient();
		/* On créer le thread pour le client avec la variable client en paramètre */
		
		/* On met la variable de fin des traitements à faux */
		termine = FALSE;
		do{
			/* On vérifie que la variable message est bien positionnée sur NULL et sinon on la met à NULL */
			if(message != NULL){
				message = NULL;
			}
			/* On récupère la requête du client */
			message = Reception(client);
			/* On teste si on a reçu quelque chose */
			if(message != NULL && strlen(message) > 0){

				/* On a bien reçu la commande du client */
				 /* On regarde si la variable commande est bien nulle et sinon on la met à NULL */
				 if(commande != NULL){
				 	commande = NULL;
				 }

				 /* On récupère la commande dans la variable commande */
				 if(sscanf(message,"%s ",commande) > 0){
				 	/* On met la commande en majuscule pour éviter les problèmes de casse */
				 	commande = putMajuscule(commande);
				 	printf("On a reçu la commande : %s\n", commande);
				 }
			}

		}while(termine != TRUE);
	}
	/* On libère le socket */
	Terminaison();
	return 0;
}

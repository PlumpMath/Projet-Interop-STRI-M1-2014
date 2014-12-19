#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "serveur.h"

#define TRUE 1
#define FALSE 0

int main(int argc, char *argv[]) {
	Client *client; /* Client connecté sur le serveur */
	int termine; /* variable qui permet de savoir si un client souhaite terminer sa session */
	char *requete; /* Requete du client */

	/* On initialise le serveur */
	InitialisationAvecService(argv[1]);

	/* On boucle en attendant les connexions des clients */
	while(1) {
		/* On récupère le client qui s'est connecté au serveur */
		client = AttenteClient();
		if(connecterClient(client) == 1){
			/* On va réaliser le traitement du client */
			do{
				termine = FALSE; /* On met termine a 0 (faux) */
				/* On récupère la requete du client */
				requete = Reception(client);
				/* On teste que la requete n'est pas vide */
				if(strcmp(requete,NULL) == 0){
					/* Si la requete est nulle on informe l'utilisateur */
					printf("ERREUR : requete nulle\n");
					Emission("ERREUR : requete nulle\n",client);
				}else{
					/* On regarde par quelle lettre commence l requete (S (stor) R (retr) Q (quit)) */
					if(requete[0] == 'S'){
						/* Demande d'envoi de fichier */
						printf("Demande d'envoi d'un fichier\n");
						recevoirFichier(client,requete);
					}else{
						if(requete[0] == 'R'){
							/* Demande de téléchargement d'un fichier */
						}else{
							if(requete[0] == 'Q'){
								/* Demande de fin de session */
								termine = TRUE;
								printf("Fin de session client\n");
								Emission("530 - Fin de connexion\n",client);
							}else{
								/* Requete inconnue */
								printf("ERREUR : requête inconnue\n");
								Emission("500 - Commande non reconnue\n",client);
								termine = TRUE;
							}
						}
					}
				}
			}while(termine != TRUE);
		}
	}

	/* On libère le socket */
	Terminaison();
	return 0;
}

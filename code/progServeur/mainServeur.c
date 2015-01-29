#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "serveur.h"

#define TRUE 1
#define FALSE 0

int main(int argc, char *argv[]) {
	Client *client; /* Client connecte sur le serveur */
	int termine; /* variable qui permet de savoir si un client souhaite terminer sa session */
	char *requete; /* Requete du client */
	char modeTransfert; /* Mode de transfert des fichiers : S (flux) , B (bloc) */

	/* On initialise le serveur */
	InitialisationAvecService(argv[1]);

	/* On boucle en attendant les connexions des clients */
	while(1) {
		/* On recupere le client qui s'est connecte au serveur */
		client = AttenteClient();
		if(connecterClient(client) == 1){
			/* Par défaut le mode de transfert est le mode flux */
			modeTransfert = 'S';
			/* On va realiser le traitement du client */
			do{
				termine = FALSE; /* On met termine a 0 (faux) */
				/* On recupere la requete du client */
				requete = Reception(client);
				printf("On a recu : %s",requete);
				/* On teste que la requete n'est pas vide */
				if(requete==NULL){
					/* Si la requete est nulle on informe l'utilisateur */
					printf("ERREUR : requete nulle\n");
					Emission("ERREUR : requete nulle\n",client);
				}else{
					/* On regarde par quelle lettre commence la requete (S (stor) R (retr) Q (quit)) */
					if(requete[0] == 'S'){
						if(requete[1] == 'T'){
							/* Demande d'envoi de fichier */
							printf("Demande d'envoi d'un fichier\n");
							recevoirFichier(client,requete);
						}
						if(requete[1] == 'I'){
							/* Demande taille d'un fichier */
							printf("Demande de la taille d'un fichier\n");
							tailleFichier(requete,client);
						}	
					}else{
						if(requete[0] == 'R'){
							if(requete[2] == 'T'){
								/* Demande de telechargement d'un fichier */
								printf("Demande de telechargement de fichier\n");
								/* on teste le mode de transfert en cour */
								if(modeTransfert == 'B'){
									/* Mode bloc */
									envoyerFichierBloc(client, requete);
								}else{
									/* Mode flux */
									envoyerFichier(client,requete);
								}
							}
							if(requete[2] == 'S'){
								/* Demande de téléchargement d'une partie d'un fichier */
								printf("Demande de téléchargement d'une partie d'un fichier\n");
								envoyerPartieFichier(client,requete);
							}

						}else{
							if(requete[0] == 'Q'){
								/* Demande de fin de session */
								termine = TRUE;
								printf("Fin de session client\n");
								Emission("530 - Fin de connexion\n",client);
							}else{
								if(requete[0] == 'M'){
									/* Changement du mode de transfert */
									printf("Changement de mode\n");
									modeTransfert = changerMode(requete,client);
								}else{
									/* Requete inconnue */
									printf("ERREUR : requete inconnue\n");
									Emission("500 - Commande non reconnue\n",client);
									termine = TRUE;
								}
							}
						}
					}
				}
			}while(termine != TRUE);
		}
	}

	/* On libere le socket */
	Terminaison();
	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "client.h"

int main(int argc, char *argv[]) {
	char requete[100]; /* requete que l'on va envoyer au serveur */
	int etatConnexion; /* 1 : connecter / 0 : non connecté */
	int choix; /* Choix pour le menu principal */

	etatConnexion = 0;

	/* On initialise le client */
	if(InitialisationAvecService(argv[1],argv[2]) == 0){
		/* Erreur sur l'initialisation, messae d'erreur afficher par la fonction */
		return -1;
	}

	/* On se connecte directement sur le serveur */
	etatConnexion = connecterUtilisateur();

	/* On vérifie que l'utilisateur est bien connecté sur le serveur */
	if(etatConnexion == 1){
		/* On va maintenant afficher le menu principal de l'application */
		do{
			/* Menu principal */
			printf("-MENU PRINCIPAL-\n\n");
			printf("1 : Envoyer un fichier sur le serveur\n");
			printf("2 : Télécharger un fichier stocké sur le serveur\n");
			printf("0 : Déconnexion\n\n");
			printf("Votre choix : ");
			fflush(stdin); /* On vide le tampon */
			/* On récupère le choix de l'utilisateur */
			if(scanf("%d",&choix) < 1){
				/* Erreur saisie */
				printf("ERREUR : votre saisie est incorrecte \n\n");
			}else{
				/* On regarde le choix de l'utilisateur */
				switch (choix){
					case 1:
						/* Envoyer un fichier */
						break;
					case 2:
						/* Télécharger un fichier */
						break;
					case 0:
						/* Quitter l'appli */
						break;
					default:
						/* Erreur saisie */
						printf("ERREUR : votre saisie est incorrecte \n\n");
						break;
				}
			}

		}while(choix != 0);

		/* On ferme l'appli */
		printf("Fin du programme, aurevoir\n");
	}

	return 0;

}
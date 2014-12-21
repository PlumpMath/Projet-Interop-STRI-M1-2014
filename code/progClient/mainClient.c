#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "client.h"


#define TRUE 1
#define FALSE 0

int main(int argc, char *argv[]) {
	char requete[100]; /* requete que l'on va envoyer au serveur */
	int etatConnexion; /* 1 : connecter / 0 : non connecté */
	int choix; /* Choix pour le menu principal */
	char nomFichier[100]; /* Nom du fichier */
	int c; /* permet de vider le buffer */
	int termine; /* permet de savoir si un client a temriné ses traitements */

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
			termine = FALSE;
			/* Menu principal */
			printf("-MENU PRINCIPAL-\n\n");
			printf("1 : Envoyer un fichier sur le serveur\n");
			printf("2 : Télécharger un fichier stocké sur le serveur\n");
			printf("0 : Déconnexion\n\n");
			printf("Votre choix : ");
			/* On récupère le choix de l'utilisateur */
			if(scanf("%d",&choix) < 1){
				/* Erreur saisie */
				printf("ERREUR : votre saisie est incorrecte \n\n");
				while ( ((c = getchar()) != '\n') && c != EOF); /* on vide le buffer */
			}else{
				while ( ((c = getchar()) != '\n') && c != EOF); /* on vide le buffer */
				/* On regarde le choix de l'utilisateur */
				switch (choix){
					case 1:
						/* Envoyer un fichier */
						printf("Saisir le nom du fichier que vous voulez envoyer (le fichier doit se trouver dans le répertoire actuel et le nom doit faire 100 caractères max) : \n");
						/* On va lire le nom du fichier au clavier */
						fgets(nomFichier,100,stdin);
						fflush(stdin); /* on vide le buffer */
						/* On vérifie que on a bien lu quelque chose */
						if(nomFichier != NULL){
							/* On lance la procédure d'envoi */
							envoyerFichier(nomFichier);
						}
						break;
					case 2:
						/* Télécharger un fichier */
						printf("Saisir le nom du fichier que vous voulez télécharger : \n");
						/* On va lire le nom du fichier au clavier */
						fgets(nomFichier,100,stdin);
						fflush(stdin); /* on vide le buffer */
						/* On vérifie que on a bien lu quelque chose */
						if(nomFichier != NULL){
							/* On lance la procédure d'envoi */
							telechargerFichier(nomFichier);
						}
						break;
					case 0:
						/* Quitter l'appli */
						Emission("QUIT\n");
						printf("%s",Reception());
						Terminaison();
						termine = TRUE;
						break;
					default:
						/* Erreur saisie */
						printf("ERREUR : votre saisie est incorrecte \n\n");
						break;
				}
			}

		}while(termine != TRUE);

		/* On ferme l'appli */
		printf("Fin du programme, aurevoir\n");
	}

	return 0;

}
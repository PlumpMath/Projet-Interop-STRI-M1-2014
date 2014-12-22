#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "client.h"


#define TRUE 1
#define FALSE 0

int main(int argc, char *argv[]) {
	char requete[100]; /* requete que l'on va envoyer au serveur */
	int etatConnexion; /* 1 : connecte / 0 : non connecte */
	int choix; /* Choix pour le menu principal */
	char nomFichier[100]; /* Nom du fichier */
	int c; /* permet de vider le buffer */
	int termine; /* permet de savoir si un client a termine ses traitements */

	etatConnexion = 0;

	/* On initialise le client */
	if(InitialisationAvecService(argv[1],argv[2]) == 0){
		/* Erreur sur l'initialisation, message d'erreur affiche par la fonction */
		return -1;
	}

	/* On se connecte directement sur le serveur */
	etatConnexion = connecterUtilisateur();

	/* On verifie que l'utilisateur est bien connecte sur le serveur */
	if(etatConnexion == 1){
		/* On va maintenant afficher le menu principal de l'application */
		do{
			termine = FALSE;
			/* Menu principal */
			printf("-MENU PRINCIPAL-\n\n");
			printf("1 : Envoyer un fichier sur le serveur\n");
			printf("2 : Telecharger un fichier stocke sur le serveur\n");
			printf("0 : Se déconnecter\n\n");
			printf("Votre choix : ");
			/* On recupere le choix de l'utilisateur */
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
						printf("Saisir le nom du fichier que vous voulez envoyer (le fichier doit se trouver dans le repertoire actuel et le nom doit faire 100 caracteres max) : \n");
						/* On va lire le nom du fichier au clavier */
						fgets(nomFichier,100,stdin);
						fflush(stdin); /* on vide le buffer */
						/* On verifie que on a bien lu quelque chose */
						if(nomFichier != NULL){
							/* On lance la procedure d'envoi */
							envoyerFichier(nomFichier);
						}
						break;
					case 2:
						/* Telecharger un fichier */
						printf("Saisir le nom du fichier que vous voulez telecharger : \n");
						/* On va lire le nom du fichier au clavier */
						fgets(nomFichier,100,stdin);
						fflush(stdin); /* on vide le buffer */
						/* On verifie qu'on a bien lu quelque chose */
						if(nomFichier != NULL){
							/* On lance la procedure d'envoi */
							telechargerFichier(nomFichier);
						}
						break;
					case 0:
						/* Quitter l'application */
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

		/* On ferme l'application */
		printf("Fin du programme, au revoir\n");
	}

	return 0;

}
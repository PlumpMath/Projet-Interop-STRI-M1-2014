#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "client.h"
#include <pthread.h>


#define TRUE 1
#define FALSE 0

int main(int argc, char *argv[]) {
	char requete[100]; /* requete que l'on va envoyer au serveur */
	int etatConnexion; /* 1 : connecte / 0 : non connecte */
	int choix; /* Choix pour le menu principal */
	char nomFichier[100]; /* Nom du fichier */
	int c; /* permet de vider le buffer */
	int termine; /* permet de savoir si un client a termine ses traitements */
	int choixModeTransfert; /* Mode de transfert du fichier, 0 = bloc / 1 = flux */

	etatConnexion = 0;
	choixModeTransfert = 1; /* Mode flux par defaut */

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
			printf("3 : Modifier le mode de telechargement des fichiers (bloc / flux)\n");
			printf("4 : Reprendre un telechargement en cours (suite a une erreur)\n");
			printf("0 : Se deconnecter\n\n");
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
							/* En fonction du mode on appelle la fonction qui correspond */
							if(choixModeTransfert == 0){
								/* On regarde si le client est lance avec plusieurs serveurs <=> nb arguments > 2 */
								if(argc > 3){
									/* on va pouvoir faire du telechargement parallele */
									pthread_t tabThread[argc-2]; /* Tableau de thread de longueur = au nombre de serveurs */ 
									int i; /* indice de parcours du tableau */
									/* On boucle pour creer les threads */
									for(i=0;i<argc-2;i++){
										donneesThread *donnees = malloc(sizeof(donneesThread));
										strcpy(donnees->numPort,argv[i+2]);
										strcpy(donnees->nomFichier,nomFichier);
										donnees->numeroServeur = i+1;
										donnees->nombreServeurs = argc-2;
										pthread_create(&tabThread[i], NULL, telechargerFichierBlocThread,(void *) donnees);
									}
									/* On boucle pour mettre le programme en attente */
									for(i=0;i<argc-2;i++){
										pthread_join(tabThread[i], NULL);
									}							
								}else{
									/* On telecharge normal depuis un seul serveur */
									telechargerFichierBloc(nomFichier);
								}
							}else{
								/* On lance la procedure d'envoi */
								telechargerFichier(nomFichier);
							}
						}
						break;
					case 3:
						/* Modification du mode de transfert */
						do{
							printf("Quel mode de transfert voulez-vous choisir :\n");
							printf("1 : Mode bloc\n");
							printf("2 : Mode flux\n");
							printf("0 : Retour\n");
							printf("Votre choix : ");
							if(scanf("%d",&choixModeTransfert) < 1){
								/* Erreur saisie */
								printf("ERREUR : votre saisie est incorrecte \n\n");
								while ( ((c = getchar()) != '\n') && c != EOF); /* on vide le buffer */
							}else{
								while ( ((c = getchar()) != '\n') && c != EOF);
								/* On teste que l'utiliateur est bien saisi 1 ou 2 ou 0 */
								switch (choixModeTransfert){
									case 1: /* Passage en mode bloc */
										changerMode('B');
										choixModeTransfert = 0;
										break;
									case 2: /* Passage en mode flux */
										changerMode('S');
										choixModeTransfert = 1;
										break;
									default:
										/* Erreur saisie */
										printf("Votre choix est incorrect\n");
								}
							}
						}while(choixModeTransfert != 0 && choixModeTransfert != 1);
						break;
					case 4:
						printf("Saisir le nom du fichier que vous voulez reprendre : \n");
						/* On va lire le nom du fichier au clavier */
						fgets(nomFichier,100,stdin);
						fflush(stdin); /* on vide le buffer */
						/* On verifie qu'on a bien lu quelque chose */
						if(nomFichier != NULL){
							/* On passe en mode bloc */
							choixModeTransfert = 0;
							/* On reprend le transfert */
							repriseTelechargement(nomFichier);
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
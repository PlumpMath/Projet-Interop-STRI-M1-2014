#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "p2p.h"
#include <pthread.h>


#define TRUE 1
#define FALSE 0

int main(int argc, char *argv[]){
	/* On recupère le numero d'ecoute */
	donneesServeur *donnees = malloc(sizeof(donneesServeur));
	strcpy(donnees->portEcoute,argv[1]);

	/* On initialise les deux threads client et serveur */
	pthread_t threadServeur;
	pthread_t threadClient;

	/* On lance les thread client et serveur pour une execution parallelle */
	if(pthread_create(&threadClient, NULL, threadModeClient,NULL) == -1){
		perror("Erreur creation thread client\n");
		return EXIT_FAILURE;
	}
	if(pthread_create(&threadServeur,NULL,threadModeServeur,(void *) donnees) == -1){
		perror("Erreur creation thread serveur\n");
		return EXIT_FAILURE;
	}

	/* Si l'utilisateur quitte l'appli (client) on termine aussi le thread serveur */
	pthread_join(threadClient,NULL);

	return 0;
}
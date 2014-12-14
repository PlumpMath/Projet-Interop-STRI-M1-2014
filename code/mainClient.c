#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "client.h"

int main(int argc, char *argv[]) {
	char *message; /* Variable qui va contenir la requete du client */
	int etatConnexion; /* 1 : connecter / 0 : non connecté */
	/* On alloue de la mémoire pour message */
	message = (char*) malloc(105);

	etatConnexion = 0;

	/* On initialise le client */
	if(InitialisationAvecService(argv[1],argv[2]) == 0){
		/* Erreur sur l'initialisation, messae d'erreur afficher par la fonction */
		return -1;
	}

	/* On se connecte directement sur le serveur */
	etatConnexion = connecterUtilisateur();

	/* On affiche un message selon que l'on soit connecté ou non */
	if(etatConnexion == 1){
		printf("connecté\n");
	}else{
		printf("non connecté\n");
	}

	return 0;

}
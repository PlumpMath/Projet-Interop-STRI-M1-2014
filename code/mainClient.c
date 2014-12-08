#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "client.h"

int main(int argc, char *argv[]) {
	char *message; /* Variable qui va contenir la requete du client */
	/* On alloue de la mémoire pour message */
	message = (char*) malloc(105);

	/* On initialise le client */
	if(InitialisationAvecService(argv[1],argv[2]) == 0){
		/* Erreur sur l'initialisation, messae d'erreur afficher par la fonction */
		return -1;
	}

	/* On demande de saisir le message pour le serveur */
	printf(Reception());

	/* on récupère la saisie */
	scanf("%[^\n]", message);

	/* On affiche le message que l'on va envoyer */
	printf("Vous avez saisi : %s\n",message);

	/* On envoi le message */
	if(Emission(message) == 0){
				/* Erreur d'émission*/
				return -2;
	}

	printf(Reception());

	return 0;

}
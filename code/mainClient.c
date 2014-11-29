#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "client.h"

int main(int argc, char *argv[]) {
	char *message = NULL; /* Variable qui va contenir le message de client a envoyé au serveur */

	/* On initialise le client */
	if(InitialisationAvecService(argv[1],argv[2]) == 0){
		/* Erreur sur l'initialisation, messae d'erreur afficher par la fonction */
		return -1;
	}else{
		/* Initialisation OK */
		printf("Initialisation du programme -> OK \n");
	}


}
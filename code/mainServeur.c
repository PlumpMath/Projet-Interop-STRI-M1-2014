#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "serveur.h"

#define TRUE 1
#define FALSE 0

int main(int argc, char *argv[]) {
	char *message = NULL; /* Message du client */
	Client *client; /* Client connecté sur le serveur */
	int termine;/* Variable permettant de savoir si l'écho a été effectué pour le client */

	/* On initialise le serveur */
	InitialisationAvecService(argv[1]);

	/* On boucle en attendant les connexions des clients */
	while(1) {
		/* On récupère le client qui s'est connecté au serveur */
		client = AttenteClient();
		if(connecterClient(client) == 0){
			Emission("530 - Connexion KO : problème sur le serveur\n",client);
		}
	}

	/* On libère le socket */
	Terminaison();
	return 0;
}

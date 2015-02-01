#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "p2p.h"
#include <pthread.h>


#define TRUE 1
#define FALSE 0

int main(int argc, char *argv[]){
	/* Variables globales */
	char uitilisateur[100]; /* Nom d'utilisateur */
	char numPortEcoute[6]; /* Numero de port ecoute (serveur) */
	strcpy(numPortEcoute,argv[1]);

	return 0;
}
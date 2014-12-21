#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <dirent.h>

#include <errno.h>

#include "serveur.h"

#define TRUE 1
#define FALSE 0

#ifdef WIN32
#define perror(x) printf("%s : code d'erreur : %d\n", (x), WSAGetLastError())
#define close closesocket
#define socklen_t int
#endif

/* Variables cachees */

/* le socket d'ecoute */
int socketEcoute;
/* longueur de l'adresse */
socklen_t longeurAdr;



/* Initialisation.
 * Creation du serveur en pr�cisant le service ou num�ro de port.
 * renvoie 1 si �a c'est bien pass� 0 sinon
 */
int InitialisationAvecService(char *service) {
	int n;
	const int on = 1;
	struct addrinfo	hints, *res, *ressave;

	#ifdef WIN32
	WSADATA	wsaData;
	if (WSAStartup(0x202,&wsaData) == SOCKET_ERROR)
	{
		printf("WSAStartup() n'a pas fonctionne, erreur : %d\n", WSAGetLastError()) ;
		WSACleanup();
		exit(1);
	}
	memset(&hints, 0, sizeof(struct addrinfo));
    #else
	bzero(&hints, sizeof(struct addrinfo));
	#endif

	/* On initialise les variables en mode connect� */
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	/* On r�cup�re les informations avec getaddrinfo() */
	if ( (n = getaddrinfo(NULL, service, &hints, &res)) != 0)  {
			/* En cas d'�chec on informe l'utilisateur */
     		printf("Initialisation, erreur de getaddrinfo : %s", gai_strerror(n));
     		return 0;
	}
	/* On r�alise ne sauvegarde des informations */
	ressave = res;

	/* On cr�er le socket d'�coute pour les clients */
	do {
		socketEcoute = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (socketEcoute < 0)
			continue;		/* error, try next one */

		setsockopt(socketEcoute, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
#ifdef BSD
		setsockopt(socketEcoute, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
#endif
		/* On associe le socket au serveur */
		if (bind(socketEcoute, res->ai_addr, res->ai_addrlen) == 0)
			break;			/* success */

		close(socketEcoute);	/* bind error, close and try next one */
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL) {
     		perror("Initialisation, erreur de bind.");
     		return 0;
	}

	/* conserve la longueur de l'addresse */
	longeurAdr = res->ai_addrlen;

	/* On lib�re la sauvegarde */
	freeaddrinfo(ressave);
	/* On cr�er une file d'attente de maximum 5 clients */
	listen(socketEcoute, 5);
	/* Success */
	return 1;
}

/* Attends qu'un client se connecte, quand un client se connecte on lance un processus fils pour le traitement de sa requ�te */
Client * AttenteClient() {
	struct sockaddr *clientAddr;
	char machine[NI_MAXHOST]; 
	Client *client; /* Client qui se connecte /

	/* On cr�er la structure client et on lui alloue de la m�moire */
	client = (Client*) malloc(sizeof(Client));
	clientAddr = (struct sockaddr*) malloc(longeurAdr);

	/* On connecte le client au socket*/
	client->socketService = accept(socketEcoute, clientAddr, &longeurAdr);

	/* On v�rifie que la connexion du client au socket s'est bien pass�e */
	if (client->socketService == -1) {
		perror("AttenteClient, erreur de accept.");
		free(clientAddr);
		free(client);
		return 0;
	}

	/* On r�cup�re les informations sur le client qui se connecte */
	if(getnameinfo(clientAddr, longeurAdr, machine, NI_MAXHOST, NULL, 0, 0) == 0) {
		printf("Client sur la machine d'adresse %s connecte.\n", machine);
	} else {
		printf("Client anonyme connecte.\n");
	}

	/* On lib�re la m�moire allou�e � l'adresse client */
	free(clientAddr);

	/*
	 * Reinit buffer
	 */
	client->debutTampon = 0;
	client->finTampon = 0;	
	return client;
}

/* Recoit un message envoye par le serveur.
 */
char *Reception(Client *client) {
	char message[LONGUEUR_TAMPON]; /* Message re�u du client */
	unsigned short int messageLength; /* Variable contenant la taille du message */
	
	/* On remplit message de z�ro pour connaitre la fin */
	memset(message,0,sizeof message);

	/* On bucle en attendant de recevoir le message du client */
	while(recv(client->socketService,message,sizeof message,0) > 0){
		messageLength = strlen(message); /* On r�cup�re la longueur du message */
		if(messageLength > 0){
			/* Si le message contient au moins 1 caract�re on le retourne */
			return message;
		}
	}
}

/* Envoie un message au client.
 */
int Emission(char *message, Client *client) {
	int taille; /* Longueur du message */

	/* On v�rifie que le message se termine bien par \n */
	if(strstr(message, "\n") == NULL) {
		/* Si le message ne finit pas par \n, on le rajoute � la fin */
		strcat(message,"\n");
	}

	/* On r�cup�re la taille du message */
	taille = strlen(message);

	/* On envoi le message au client */
	if (send(client->socketService, message, taille,0) == -1) {
        perror("Emission, probleme lors du send.\n");
        return 0;
	}

	return 1;
}


/* Ferme la connexion avec le client.
 */
void TerminaisonClient(Client *client) {
	close(client->socketService);
	free(client);
}

/* Arrete le serveur.
 */
void Terminaison() {
	close(socketEcoute);
}

/* Met tous les caract�res d'une cha�ne en majuscule */
char * putMajuscule(char *ch){
    int i; /* indice de parcour de la cha�ne */
    for(i=0;i<strlen(ch)-1;i++){
        /* pour chaque caract�res de ch on le remplace par la minuscule correspondante */
        ch[i] = toupper(ch[i]);
    }
    return ch;
}

/* Liste le r�pertoire pass� en param�tre, les diff�rents �l�ments seront s�par�s par des # */
char * listeDir(char *repCourrant){


	DIR* dir = NULL; /* Dossier ouvert */
	struct dirent* fic = NULL; /* fichier s�lectionn� */
	char * nomFic = NULL;
	char * listeFic = NULL; /* Liste des des fichiers pr�sents dans le dossier */

	/* On ouvre le r�pertoire pass� en param�tre */
	dir = opendir(repCourrant);


	/* On v�rifie que le dossier a bien �t� ouvert */
	if(dir == NULL){
		perror("");
		return NULL;
	}


	/* On va lister l'ensemble du r�pertoire */
	while((fic = readdir(dir)) != NULL){
		nomFic = NULL;
		nomFic = fic->d_name;
		if(strchr(nomFic,'.') == NULL){
			printf("Dossier = %s\n",nomFic);
		}else{
			if(strcmp(nomFic,".") != 0){
				if(strcmp(nomFic,"..") != 0){
					printf("Fichier = %s\n",nomFic);
				}
			}
		}
	}

	/* On renvoi la liste des fichiers */
	return listeFic;
}

/* R�alise la connexion du client en param�tre sur le serveur FTP 
retourne 1 si client connecte et 0 sinon*/
int connecterClient(Client *client){
	char *message = NULL; /* Message du client */
	char * messageSave;
	char *requete = NULL; /* Requete client */
	char *utilisateur = NULL; /* Nom d'utilisateur du client */

	printf("Demande de connexion client\n");

	/* On demande le nom d'utilisateur au client */
	printf("On demande le nom d'utilisateur\n");
	Emission("220 - Saisir utilisateur (max 50 caract�res) : \n", client);
	/* On r�cup�re la r�ponse du client qui doit �tre sous forme USER utilisateur */
	message = Reception(client);
	/* On alloue de la m�moire a la variable de sauvegarde pour pouvoir sauvegarder le message */
	messageSave = (char*) malloc(60);
	/* On sauvegarde le message re�u */
	strcpy(messageSave,message);
	/* On analyse le message du client pour voir si il est conforme */
	/* On d�coupe le message du client pour extraire les informations */
	requete = strtok(message, " \n");
	utilisateur = strtok(NULL," \n");
	/* On teste que la requete est bien USER */
	if(strcmp(requete,"USER") != 0){
		/* requete incorrecte */
		printf("Requ�te incorrecte : mauvaise commande\n");
		Emission("500 - Requ�te incorrecte\n",client);
		return 0;
	}else{
		/* On teste que l'utilisateur n'est pas NULL */
		if(utilisateur == NULL || strcmp(utilisateur,"") == 0){
			printf("Utilisateur NULL ou vide\n");
			Emission("501 - Utilisateur NULL ou vide\n",client);
			return 0;
		}else{
			/* On teste maintenant que la requ�te n'est pas trop longue */
			/* On cr�er une requete de test qui est de la bonne longueur */
			char *testLongueurMessage;
			/* On lui alloue de la m�moire */
			testLongueurMessage = (char*) malloc(60);
			sprintf(testLongueurMessage,"USER %s\n",utilisateur);
			/* On teste maintenant si la requete que l'on a re�u du client fait la bonne longueur */
			if(strlen(testLongueurMessage) != strlen(messageSave)){
				/* requete incorrecte */
				printf("Requ�te incorrecte : probl�me de longueur\n");
				Emission("500 - Requ�te incorrecte\n",client);
				return 0;
			}else{
				/* On autorise la connexion du client sur le serveur */
				printf("Connexion autorisee pour l'utilisateur %s\n",utilisateur);
				Emission("230 : Connexion �tablie\n",client);
				return 1;
			}
		}
	}
	
}

/* Permet au client d'envoyer un fichier sur le serveur, si le fichier est d�j� pr�sent sur le serveur on �crase */
void recevoirFichier(Client *client, char *requete){
	char *requeteSave; /* sauvegarde de la requete pass�e en param�tre */
	char *nomFichier; /* nom du fichier envoy� par l'utilisateur */
	char *commande; /* commande de l'utilisateur */
	char fichierSave[100]; /* sauvegarde du nom du fichier */

	/* On alloue de la m�moire � la sauvegarde de la requete */
	requeteSave = (char*) malloc(100);
	/* On sauvegarde la requete */
	strcpy(requeteSave,requete);
	/* On d�compose la requete client pour extraire le nom du fichier et la commande */
	commande = strtok(requete, " \n");
	nomFichier = strtok(NULL, " \n");
	/* on sauvegarde le nom du fichier */
	strcpy(fichierSave,nomFichier);

	/* On v�rifie que la commande est bien STOR */
	if(strcmp(commande,"STOR") != 0){
		/* On envoi l'erreur au client */
		printf("Requete incorrecte : mauvaise commande\n");
		Emission("500 - Requ�te incorrecte\n",client);
	}else{
		/* On teste que le chemin du fichier pas vide */
		if(nomFichier == NULL || strcmp(nomFichier,"") == 0){
			/* Erreur pas de chemin pour le fichier */
			printf("ERREUR : Le chemin du fichier est vide\n");
			Emission("501 - Chemin du fichier NULL ou vide\n",client);
		}else{
			/* On teste maintenant la longueur de la requete */
			/* On va donc comparer la requete sauvegard�e � une requete que l'on monte pour le test */
			sprintf(requete,"STOR %s\n",nomFichier);
			if(strlen(requeteSave) != strlen(requete)){
				/* requete incorrecte */
				printf("Requ�te incorrecte : probl�me de longueur\n");
				Emission("500 - Requ�te incorrecte\n",client);
			}else{
				/* On demande maintenant le contenu du fichier au client */
				Emission("150 - Transfert autoris�\n",client);
				char *contenuFichier = NULL;
				contenuFichier = Reception(client);

				/* On teste que le contenu a bien �t� re�u */
				if(contenuFichier == NULL){
					printf("ERREUR : probl�me r�ception contenu fichier\n");
					Emission("451 - Erreur avec l'envoi du fichier\n",client);
				}else{
					/* On va maintenant cr�er le fichier */
					FILE * fichier = NULL; /* Fichier dans lequel on va �crire */
					strcpy(nomFichier,fichierSave); /* on r�cup�re la sauveharde du nom du fichier */
					fichier = fopen(nomFichier,"wb");
					/* On teste la cr�ation/ouverture du fichier */
					if(fichier == NULL){
						/* Probleme cr�ation fichier */
						printf("ERREUR : cr�ation fichier KO\n");
						Emission("451 - Erreur avec la cr�ation du fichier sur le serveur\n",client);
					}else{
						printf("%s\n",contenuFichier);
						/* On va maintenant �crire le contenu dans la fichier */
						if(fwrite(contenuFichier,strlen(contenuFichier),1,fichier) < 1){
				    		/* Erreur �criture du fichier */
				        	printf("ERREUR : ecriture fichier KO\n");
				        	/* on ferme le fichier */
				    		fclose(fichier);
				        	Emission("451 - Erreur avec l'�criture dans le fichier sur le serveur\n",client);
				    	}else{
				    		/* Si tout c'est bien on informe l'utilisateur */
				    		printf("Transfert du fichier OK\n");
				    		/* on ferme le fichier */
				    		fclose(fichier);
				    		Emission("226 - Transfert du fichier termine\n",client);
				    		
				    	}
					}
				}
			}
		}
	}

}

/* Permet au serveur d'envoyer un fichier � un client qui en fait la demande */
int envoyerFichier(Client *client, char *requete){
	char *requeteSave; /* sauvegarde de la requete pass�e en param�tre */
	char *nomFichier; /* nom du fichier � envoyer */
	char *commande; /* commande de l'utilisateur */
	FILE * fichier = NULL; /* fichier � envoyer */
	char *contenuFichier; /* Contenu du fichier que l'on veut envoyer */
	long taille; /* taille du fichier */
	char fichierSave[100]; /* sauvegarde du nom du fichier */
	char *resultatTelechargement; /* Ok ou KO en fonction du r�sultat du t�l�chargement */

	/* On alloue de la m�moire � la sauvegarde de la requete */
	requeteSave = (char*) malloc(100);

	/* On alloue de la m�moire pour la variable */
	resultatTelechargement = (char*) malloc(5);

	/* On sauvegarde la requete */
	strcpy(requeteSave,requete);

	/* On d�compose la requete client pour extraire le nom du fichier et la commande */
	commande = strtok(requete, " \n");
	nomFichier = strtok(NULL, " \n");

	/* on sauvegarde le nom du fichier */
	strcpy(fichierSave,nomFichier);

	/* On v�rifie que la commande est bien RETR */
	if(strcmp(commande,"RETR") != 0){
		/* On envoi l'erreur au client */
		printf("Requete incorrecte : mauvaise commande\n");
		Emission("500 - Requ�te incorrecte\n",client);
		return 0;
	}else{
		/* On teste que le chemin du fichier pas vide */
		if(nomFichier == NULL || strcmp(nomFichier,"") == 0){
			/* Erreur pas de chemin pour le fichier */
			printf("ERREUR : Le chemin du fichier est vide\n");
			Emission("501 - Chemin du fichier NULL ou vide\n",client);
			return 0;
		}else{
			/* On teste maintenant la longueur de la requete */
			/* On va donc comparer la requete sauvegard�e � une requete que l'on monte pour le test */
			sprintf(requete,"RETR %s\n",nomFichier);
			if(strlen(requeteSave) != strlen(requete)){
				/* requete incorrecte */
				printf("Requ�te incorrecte : probl�me de longueur\n");
				Emission("500 - Requ�te incorrecte\n",client);
				return 0;
			}else{
				/* On va maintenant ouvrir le fichier demand� */
				strcpy(nomFichier,fichierSave); /* on r�cup�re la sauveharde du nom du fichier */
				/* Ouverture du fichier en mode lecture */
				fichier = fopen(nomFichier,"rb");
				/* On teste l'ouverture du fichier */
				if(fichier == NULL){
					/* Erreur ouverture fichier */
					printf("ERREUR : ouverture du fichier impossible\n");
					Emission("550 - Impossible d'ouvrir le fichier\n",client);
					return 0;
				}else{
					/* on r�cup�re le contenu du fichier */
					/* On r�cup�re la taille du fichier */
					fseek (fichier , 0 , SEEK_END);
			  		taille = ftell (fichier);
			  		rewind (fichier);
					/* On alloue de la m�moire pour le contenu du fichier */
					contenuFichier = (char*) malloc(taille);
					/* on va maintenant lire le contenu du fichier */
					if(fread(contenuFichier,1,taille,fichier)<1){
						/* Erreur lecture fichier */
						printf("ERREUR : lecture du fichier �chou�e\n");
						/* on ferme le fichier */
				    		fclose(fichier);
						Emission("550 - Impossible de lire le fichier\n",client);
						return 0;
					}else{
						printf("1\n");
						/* On teste que contenu fichier est non null */
						if(contenuFichier == NULL){
							/* Contenu fichier null */
							printf("ERREUR : contenu du fichier null\n");
							/* on ferme le fichier */
				    		fclose(fichier);
							Emission("550 - Impossible de lire le fichier\n",client);
							return 0;
						}else{
							/* on ferme le fichier */
				    		fclose(fichier);
							/* On informe que le t�l�chargement va d�buter */
							Emission("150 - D�but du t�l�chargement\n",client);
							/* On envoi le contenu du fichier au client */
							printf("Envoi du contenu fichier\n");
							Emission(contenuFichier,client);
							printf("T�l�chargement OK\n");

							/* On r�cup�re la r�ponse avec le resultat du t�l�chargement */
							resultatTelechargement = Reception(client);
							if(strstr(resultatTelechargement,"OK") != NULL){
								/* On envoi le message 226 */
								Emission("226 - T�l�chargement termin�\n",client);
							}else{
								/* On envoi 451 */
								Emission("451 - T�l�chargement �chou�\n",client);
							}
							return 1;
						}
					}
				}
			}
		}
	}
}
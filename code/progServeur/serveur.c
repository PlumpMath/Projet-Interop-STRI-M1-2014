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
 * Creation du serveur en precisant le service ou numero de port.
 * renvoie 1 si ca c'est bien passe 0 sinon
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

	/* On initialise les variables en mode connecte */
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	/* On recupere les informations avec getaddrinfo() */
	if ( (n = getaddrinfo(NULL, service, &hints, &res)) != 0)  {
			/* En cas d'echec on informe l'utilisateur */
     		printf("Initialisation, erreur de getaddrinfo : %s", gai_strerror(n));
     		return 0;
	}
	/* On realise une sauvegarde des informations */
	ressave = res;

	/* On cree le socket d'ecoute pour les clients */
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

	/* conserve la longueur de l'adresse */
	longeurAdr = res->ai_addrlen;

	/* On libere la sauvegarde */
	freeaddrinfo(ressave);
	/* On cree une file d'attente de maximum 5 clients */
	listen(socketEcoute, 5);
	/* Success */
	return 1;
}

/* Attends qu'un client se connecte, quand un client se connecte on lance un processus fils pour le traitement de sa requete */
Client * AttenteClient() {
	struct sockaddr *clientAddr;
	char machine[NI_MAXHOST]; 
	Client *client; /* Client qui se connecte /

	/* On cree la structure client et on lui alloue de la memoire */
	client = (Client*) malloc(sizeof(Client));
	clientAddr = (struct sockaddr*) malloc(longeurAdr);

	/* On connecte le client au socket*/
	client->socketService = accept(socketEcoute, clientAddr, &longeurAdr);

	/* On verifie que la connexion du client au socket s'est bien passee */
	if (client->socketService == -1) {
		perror("AttenteClient, erreur de accept.");
		free(clientAddr);
		free(client);
		return 0;
	}

	/* On recupere les informations sur le client qui se connecte */
	if(getnameinfo(clientAddr, longeurAdr, machine, NI_MAXHOST, NULL, 0, 0) == 0) {
		printf("Client sur la machine d'adresse %s connecte.\n", machine);
	} else {
		printf("Client anonyme connecte.\n");
	}

	/* On libere la memoire allouee a l'adresse client */
	free(clientAddr);

	/*
	 * Reinitialisation buffer
	 */
	client->debutTampon = 0;
	client->finTampon = 0;	
	return client;
}

/* Recoit un message envoye par le serveur.
 */
char *Reception(Client *client) {
	char message[LONGUEUR_TAMPON]; /* Message recu du client */
	unsigned short int messageLength; /* Variable contenant la taille du message */
	
	/* On remplit message de zero pour connaitre la fin */
	memset(message,0,sizeof message);

	/* On boucle en attendant de recevoir le message du client */
	while(recv(client->socketService,message,sizeof message,0) > 0){
		messageLength = strlen(message); /* On recupere la longueur du message */
		if(messageLength > 0){
			/* Si le message contient au moins 1 caractere on le retourne */
			return strdup(message);
		}
	}
}

/* Envoie un message au client.
 */
int Emission(char *message, Client *client) {
	int taille; /* Longueur du message */

	/* On verifie que le message se termine bien par \n */
	if(strstr(message, "\n") == NULL) {
		/* Si le message ne finit pas par \n, on le rajoute a la fin */
		strcat(message,"\n");
	}

	/* On recupere la taille du message */
	taille = strlen(message);

	/* On envoie le message au client */
	if (send(client->socketService, message, taille,0) == -1) {
        perror("Emission, probleme lors du send.\n");
        return 0;
	}

	return 1;
}

/* Envoie des donnees au client en precisant leur taille.
 */
int EmissionBinaire(char *donnees, size_t taille, Client *client) {
	int retour = 0;
	retour = send(client->socketService, donnees, taille, 0);
	if(retour == -1) {
		perror("Emission, probleme lors du send.");
		return -1;
	} else {
		return retour;
	}
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

/* Met tous les caracteres d'une chaine en majuscule */
char * putMajuscule(char *ch){
    int i; /* indice de parcour de la chaine */
    for(i=0;i<strlen(ch)-1;i++){
        /* pour chaque caracteres de ch on le remplace par la minuscule correspondante */
        ch[i] = toupper(ch[i]);
    }
    return ch;
}

/* Liste le repertoire passe en parametre, les differents elements seront separes par des # */
char * listeDir(char *repCourrant){


	DIR* dir = NULL; /* Dossier ouvert */
	struct dirent* fic = NULL; /* Fichier selectionne */
	char * nomFic = NULL;
	char * listeFic = NULL; /* Liste des fichiers presents dans le dossier */

	/* On ouvre le repertoire passe en parametre */
	dir = opendir(repCourrant);


	/* On verifie que le dossier a bien ete ouvert */
	if(dir == NULL){
		perror("");
		return NULL;
	}


	/* On va lister l'ensemble du repertoire */
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

	/* On renvoie la liste des fichiers */
	return listeFic;
}

/*
Parametres : str : chaine principale / len : longueur de la sous-chaine / pos : debut de la sous-chaine  
Extrait la sous-chaine de longueur "len" a partir du caractere numero "pos" dans la chaine "str" 
*/
char *extraireSousChaine(char *str, long len, long pos){
	long i; /* indice de parcours de la chaine */
	char sousChaine[len]; /* Sous-chaine que l'on va retourner */

	/* On se positionne sur le debut de la sous-chaine et on recupere les i caracteres */
	for(i=pos;i<(pos+len);i++){
		sousChaine[i-pos] = str[i];
	}

	/* On retourne la sous-chaine */
	return strdup(sousChaine);
}

/* Realise la connexion du client en parametre sur le serveur FTP 
retourne 1 si client connecte et 0 sinon*/
int connecterClient(Client *client){
	char *message = NULL; /* Message du client */
	char * messageSave;
	char *requete = NULL; /* Requete client */
	char *utilisateur = NULL; /* Nom d'utilisateur du client */

	printf("Demande de connexion client\n");

	/* On demande le nom d'utilisateur au client */
	printf("On demande le nom d'utilisateur\n");
	Emission("220 - Saisir utilisateur (max 50 caracteres) : \n", client);
	/* On recupere la reponse du client qui doit etre sous forme USER utilisateur */
	message = Reception(client);
	/* On alloue de la memoire a la variable de sauvegarde pour pouvoir sauvegarder le message */
	messageSave = (char*) malloc(60);
	/* On sauvegarde le message recu */
	strcpy(messageSave,message);
	/* On analyse le message du client pour voir s'il est conforme */
	/* On decoupe le message du client pour extraire les informations */
	requete = strtok(message, " \n");
	utilisateur = strtok(NULL," \n");
	/* On teste que la requete est bien USER */
	if(strcmp(requete,"USER") != 0){
		/* requete incorrecte */
		printf("Requete incorrecte : mauvaise commande\n");
		Emission("500 - Requete incorrecte\n",client);
		return 0;
	}else{
		/* On teste que l'utilisateur n'est pas NULL */
		if(utilisateur == NULL || strcmp(utilisateur,"") == 0){
			printf("Utilisateur NULL ou vide\n");
			Emission("501 - Utilisateur NULL ou vide\n",client);
			return 0;
		}else{
			/* On teste maintenant que la requete n'est pas trop longue */
			/* On cree une requete de test qui est de la bonne longueur */
			char *testLongueurMessage;
			/* On lui alloue de la memoire */
			testLongueurMessage = (char*) malloc(60);
			sprintf(testLongueurMessage,"USER %s\n",utilisateur);
			/* On teste maintenant si la requete que l'on a recu du client fait la bonne longueur */
			if(strlen(testLongueurMessage) != strlen(messageSave)){
				/* requete incorrecte */
				printf("Requete incorrecte : probleme de longueur\n");
				Emission("500 - Requete incorrecte\n",client);
				return 0;
			}else{
				/* On autorise la connexion du client sur le serveur */
				printf("Connexion autorisee pour l'utilisateur %s\n",utilisateur);
				Emission("230 : Connexion etablie\n",client);
				return 1;
			}
		}
	}
	
}

/* Permet au client d'envoyer un fichier sur le serveur, si le fichier est deja present sur le serveur on ecrase */
void recevoirFichier(Client *client, char *requete){
	char *requeteSave; /* sauvegarde de la requete passee en parametre */
	char *nomFichier; /* nom du fichier envoye par l'utilisateur */
	char *commande; /* commande de l'utilisateur */
	char fichierSave[100]; /* sauvegarde du nom du fichier */

	/* On alloue de la memoire a la sauvegarde de la requete */
	requeteSave = (char*) malloc(100);
	/* On sauvegarde la requete */
	strcpy(requeteSave,requete);
	/* On decompose la requete client pour extraire le nom du fichier et la commande */
	commande = strtok(requete, " \n");
	nomFichier = strtok(NULL, " \n");
	/* on sauvegarde le nom du fichier */
	strcpy(fichierSave,nomFichier);

	/* On verifie que la commande est bien STOR */
	if(strcmp(commande,"STOR") != 0){
		/* On envoie l'erreur au client */
		printf("Requete incorrecte : mauvaise commande\n");
		Emission("500 - Requete incorrecte\n",client);
	}else{
		/* On teste que le chemin du fichier pas vide */
		if(nomFichier == NULL || strcmp(nomFichier,"") == 0){
			/* Erreur pas de chemin pour le fichier */
			printf("ERREUR : Le chemin du fichier est vide\n");
			Emission("501 - Chemin du fichier NULL ou vide\n",client);
		}else{
			/* On teste maintenant la longueur de la requete */
			/* On va donc comparer la requete sauvegardee a une requete que l'on monte pour le test */
			sprintf(requete,"STOR %s\n",nomFichier);
			if(strlen(requeteSave) != strlen(requete)){
				/* requete incorrecte */
				printf("Requete incorrecte : probleme de longueur\n");
				Emission("500 - Requete incorrecte\n",client);
			}else{
				/* On demande maintenant le contenu du fichier au client */
				Emission("150 - Transfert autorise\n",client);
				char *contenuFichier = NULL;
				contenuFichier = Reception(client);

				/* On teste que le contenu a bien ete recu */
				if(contenuFichier == NULL){
					printf("ERREUR : probleme reception contenu fichier\n");
					Emission("451 - Erreur avec l'envoi du fichier\n",client);
				}else{
					/* On va maintenant cree le fichier */
					FILE * fichier = NULL; /* Fichier dans lequel on va ecrire */
					strcpy(nomFichier,fichierSave); /* on recupere la sauvegarde du nom du fichier */
					fichier = fopen(nomFichier,"wb");
					/* On teste la creation/ouverture du fichier */
					if(fichier == NULL){
						/* Probleme creation fichier */
						printf("ERREUR : creation fichier KO\n");
						Emission("451 - Erreur avec la creation du fichier sur le serveur\n",client);
					}else{
						printf("%s\n",contenuFichier);
						/* On va maintenant ecrire le contenu dans la fichier */
						if(fwrite(contenuFichier,strlen(contenuFichier),1,fichier) < 1){
				    		/* Erreur ecriture du fichier */
				        	printf("ERREUR : ecriture fichier KO\n");
				        	/* on ferme le fichier */
				    		fclose(fichier);
				        	Emission("451 - Erreur avec l'ecriture dans le fichier sur le serveur\n",client);
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

/* Permet au serveur d'envoyer un fichier a un client qui en fait la demande */
int envoyerFichier(Client *client, char *requete){
	char *requeteSave; /* sauvegarde de la requete passee en parametre */
	char *nomFichier; /* nom du fichier a envoyer */
	char *commande; /* commande de l'utilisateur */
	FILE * fichier = NULL; /* fichier a envoyer */
	char *contenuFichier; /* Contenu du fichier que l'on veut envoyer */
	long taille; /* taille du fichier */
	char fichierSave[100]; /* sauvegarde du nom du fichier */
	char *resultatTelechargement; /* Ok ou KO en fonction du resultat du telechargement */

	/* On alloue de la memoire a la sauvegarde de la requete */
	requeteSave = (char*) malloc(100);

	/* On alloue de la memoire pour la variable */
	resultatTelechargement = (char*) malloc(5);

	/* On sauvegarde la requete */
	strcpy(requeteSave,requete);

	/* On decompose la requete client pour extraire le nom du fichier et la commande */
	commande = strtok(requete, " \n");
	nomFichier = strtok(NULL, " \n");

	/* on sauvegarde le nom du fichier */
	strcpy(fichierSave,nomFichier);

	/* On verifie que la commande est bien RETR */
	if(strcmp(commande,"RETR") != 0){
		/* On envoie l'erreur au client */
		printf("Requete incorrecte : mauvaise commande\n");
		Emission("500 - Requete incorrecte\n",client);
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
			/* On va donc comparer la requete sauvegardee a une requete que l'on monte pour le test */
			sprintf(requete,"RETR %s\n",nomFichier);
			if(strlen(requeteSave) != strlen(requete)){
				/* requete incorrecte */
				printf("Requete incorrecte : probleme de longueur\n");
				Emission("500 - Requete incorrecte\n",client);
				return 0;
			}else{
				/* On va maintenant ouvrir le fichier demande */
				strcpy(nomFichier,fichierSave); /* on recupere la sauvegarde du nom du fichier */
				/* Ouverture du fichier en mode lecture */
				fichier = fopen(nomFichier,"rb");
				/* On teste l'ouverture du fichier */
				if(fichier == NULL){
					/* Erreur ouverture fichier */
					printf("ERREUR : ouverture du fichier impossible\n");
					Emission("550 - Impossible d'ouvrir le fichier\n",client);
					return 0;
				}else{
					/* on recupere le contenu du fichier */
					/* On recupere la taille du fichier */
					fseek (fichier , 0 , SEEK_END);
			  		taille = ftell (fichier);
			  		rewind (fichier);
					/* On alloue de la memoire pour le contenu du fichier */
					contenuFichier = (char*) malloc(taille);
					/* on va maintenant lire le contenu du fichier */
					if(fread(contenuFichier,1,taille,fichier)<1){
						/* Erreur lecture fichier */
						printf("ERREUR : lecture du fichier echouee\n");
						/* on ferme le fichier */
				    	fclose(fichier);
						Emission("550 - Impossible de lire le fichier\n",client);
						return 0;
					}else{
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
							/* On informe que le telechargement va debuter */
							Emission("150 - Debut du telechargement\n",client);
							/* On envoie le contenu du fichier au client */
							printf("Envoi du contenu fichier\n");
							Emission(contenuFichier,client);
							printf("Telechargement OK\n");

							/* On recupere la reponse avec le resultat du telechargement */
							resultatTelechargement = Reception(client);
							if(strstr(resultatTelechargement,"OK") != NULL){
								/* On envoie le message 226 */
								Emission("226 - Telechargement termine\n",client);
							}else{
								/* On envoie 451 */
								Emission("451 - Telechargement echoue\n",client);
							}
							return 1;
						}
					}
				}
			}
		}
	}
}


/* Envoi en mode bloc, retourne 1 si ok et 0 sinon */
int envoyerFichierBloc(Client *client, char *requete){
	FILE * fichier; /* Fichier que l'on veut envoyer */
	char *nomFichier; /* Chemin du fichier que l'on veut envoyer */
	char *requeteSave; /* Sauvegarde de la requete client pour le test de longueur */
	char fichierSave[100]; /* Sauvegarde du nom du fichier car il s'efface au cours de l'execution */
	char *commande; /* Commande de l'utilisateur */
	char caractereCourrant; /* Caractere lu dans le fichier */
	int compteur; /* Compteur de caractere */
	char donnees[8191]; /* Donnees du bloc */
	char *retourClient; /* Reponse du client apres envoi d'un bloc */
	char *tailleDejaRecue; /* Depart de la reprise */

	/* On alloue de la memoire a la sauvegarde de la requete */
	requeteSave = (char*) malloc(100);

	/* On sauvegarde la requete */
	strcpy(requeteSave,requete);

	/* On decompose la requete client pour extraire le nom du fichier et la commande */
	commande = strtok(requete, " \n");
	nomFichier = strtok(NULL, " \n");

	/* on sauvegarde le nom du fichier */
	strcpy(fichierSave,nomFichier);

	/* On verifie que la commande est bien RETR */
	if(strcmp(commande,"RETR") != 0){
		/* On envoie l'erreur au client */
		printf("Requete incorrecte : mauvaise commande\n");
		Emission("500 - Requete incorrecte\n",client);
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
			/* On va donc comparer la requete sauvegardee a une requete que l'on monte pour le test */
			sprintf(requete,"RETR %s\n",nomFichier);
			if(strlen(requeteSave) != strlen(requete)){
				/* requete incorrecte */
				printf("Requete incorrecte : probleme de longueur\n");
				Emission("500 - Requete incorrecte\n",client);
				return 0;
			}else{
				/* On recupere le nom du fichier de la sauvegarde */
				strcpy(nomFichier,fichierSave);
				/* On ouvre le fichier en mode binaire */
				fichier = NULL; /* on met fichier a NULL pour bien pouvoir tester l'ouverture */
				fichier = fopen(nomFichier,"rb");
				/* On teste l'ouverture du fichier */
				if(fichier == NULL){
					/* Echec de l'ouverture */
					printf("ERREUR : ouverture du fichier impossible\n");
					Emission("550 - Impossible d'ouvrir le fichier\n",client);
					return 0;
				}else{
					/* On informe le client que le telechargement va commencer */
					Emission("150 - Debut du telechargement\n",client);
					/* On positionne le compteur de caractere a 0 */
					compteur = 0;
					/* On va lire le fichier caractere par caractere */
					do{
						/* On recupere le premier caractere */
						caractereCourrant = fgetc(fichier);
						/* Si on a atteint la fin du fichier, on prepare le bloc et on envoie */
						if(caractereCourrant == EOF){
							unsigned char descripteur = 0;
							EmissionBinaire(&descripteur,1,client);
							unsigned short taille = htons(compteur); /* ntohs cote reception */
							EmissionBinaire((char*)&taille,2,client);
							EmissionBinaire(donnees,compteur,client);
							retourClient = Reception(client);
							if(strstr(retourClient,"OK") == NULL){
								/* On regarde si le client demande la reprise */
								if(strstr(retourClient,"REST") != NULL){
									/* On recupere la taille deja recue */
									tailleDejaRecue = (char*) malloc(strlen(retourClient)-6);
									int x; /* indice de parcours */
									for(x=5;x<strlen(retourClient);x++){
										tailleDejaRecue[x-5] = retourClient[x];
									}
									/* On positionne le curseur dans le fichier a l'emplacement de la reprise */
									fseek(fichier,atoi(tailleDejaRecue),SEEK_SET);
								}else{
									printf("Erreur client\n");
									return 0;
								}
								
							}
						}else{
							/* On ajoute le caractere a la partie des donnees du bloc */
							donnees[compteur] = caractereCourrant;
							/* On incremente le compteur */
							compteur++;
							/* Si le compteur atteint la taille du bloc, on prepare le bloc et on envoie le bloc */
							if(compteur == 8191){
								unsigned char descripteur = 0;
								EmissionBinaire(&descripteur,1,client);
								unsigned short taille = htons(8191); /* ntohs cote reception */
								EmissionBinaire((char*)&taille,2,client);
								EmissionBinaire(donnees,8191,client);
								retourClient = Reception(client);
								if(strstr(retourClient,"OK") == NULL){
									/* On regarde si le client demande la reprise */
									if(strstr(retourClient,"REST") != NULL){
										/* On recupere la taille deja recue */
										tailleDejaRecue = (char*) malloc(strlen(retourClient)-6);
										int x; /* indice de parcours */
										for(x=5;x<strlen(retourClient);x++){
											tailleDejaRecue[x-5] = retourClient[x];
										}
										/* On positionne le curseur dans le fichier a l'emplacement de la reprise */
										fseek(fichier,atoi(tailleDejaRecue),SEEK_SET);
									}else{
										printf("Erreur client\n");
										return 0;
									}
									
								}
								/* On remet le compteur a 0 */
								compteur = 0;
							}
						}
					}while(caractereCourrant != EOF);
					unsigned char descripteur = 64;
					EmissionBinaire(&descripteur,1,client);
					unsigned short taille = 0; /* ntohs cote reception */
					EmissionBinaire((char*)&taille,2,client);
					/* On teste le retour client et on envoie le message correspondant */
					if(strstr(Reception(client),"OK") != NULL){
						/* le retour client contient bien OK */
						printf("Telechargement OK\n");
						Emission("226 - Telechargement termine\n",client);
					}else{
						printf("Telechargement KO\n");
						Emission("451 - Telechargement echoue\n",client);
					}
					/* On quitte la fonction avec le code retour 1 */
					return 1;
				}
			}
		}
	}
}

/* Change le mode de transfert des fichier, retourne NULL si KO ou le codeMode si OK */
char changerMode(char *requete, Client *client){
	char mode; /* code pour le mode demande */
	/* On recupere dans la requete le caractere correspondant au mode, si la requete est bien formee c'est le 6eme caractere, S = flux et B = bloc */
	mode = requete[5];
	/* On teste que le mode demande est bien correct */
	if(mode == 'B' || mode == 'S'){
		/* Le mode est correct on teste maintenant la longueur de la requete, typiquement MODE CODE\n => 7 caracteres */
		if(strlen(requete) == 7){
			printf("%s\n",extraireSousChaine(requete, 5, 0));
			/* Requete correcte, on teste maintenant la commande */
			if(strcmp(extraireSousChaine(requete, 5, 0),"MODE ") == 0){
				/* La commande est correcte on retourne le nouveau mode */
				Emission("200 - Changement de mode de transfert termine\n",client);
				return mode;
			}else{
				/* Erreur commande incorrecte */
				Emission("500 - Commande inconnue\n",client);
				return 0;
			}
		}else{
			/* Longueur de requete incorrecte */
			Emission("500 - Erreur de syntaxe dans la requete, longueur incorrecte\n",client);
			return 0;
		}
	}else{
		/* Mode incorrect */
		Emission("501 - Le mode demande est incorrect\n",client);
		return 0;
	}
}

/* Renvoi au client la taille du fichier qu'il donne en parametre */
int tailleFichier(char *requete, Client *client){
	FILE * fichier; /* Fichier que l'on veut envoyer */
	char *nomFichier; /* Chemin du fichier que l'on veut envoyer */
	char *requeteSave; /* Sauvegarde de la requete client pour le test de longueur */
	char fichierSave[100]; /* Sauvegarde du nom du fichier car il s'efface au cours de l'execution */
	char *commande; /* Commande de l'utilisateur */
	long taille; /* Taille du fichier */
	char reponse[100]; /* Reponse pour le client */


	/* On alloue de la memoire a la sauvegarde de la requete */
	requeteSave = (char*) malloc(100);


	/* On sauvegarde la requete */
	strcpy(requeteSave,requete);

	/* On decompose la requete client pour extraire le nom du fichier et la commande */
	commande = strtok(requete, " \n");
	nomFichier = strtok(NULL, " \n");

	/* on sauvegarde le nom du fichier */
	strcpy(fichierSave,nomFichier);

	/* On verifie que la commande est bien SIZE */
	if(strcmp(commande,"SIZE") != 0){
		/* On envoie l'erreur au client */
		printf("Requete incorrecte : mauvaise commande\n");
		Emission("500 - Requete incorrecte\n",client);
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
			/* On va donc comparer la requete sauvegardee a une requete que l'on monte pour le test */
			sprintf(requete,"SIZE %s\n",nomFichier);
			if(strlen(requeteSave) != strlen(requete)){
				/* requete incorrecte */
				printf("Requete incorrecte : probleme de longueur\n");
				Emission("500 - Requete incorrecte\n",client);
				return 0;
			}else{
	
				/* On recupere le nom du fichier de la sauvegarde */
				strcpy(nomFichier,fichierSave);
				/* On ouvre le fichier en mode binaire */
				fichier = NULL; /* on met fichier a NULL pour bien pouvoir tester l'ouverture */
				fichier = fopen(nomFichier,"rb");
				/* On teste l'ouverture du fichier */
				if(fichier == NULL){
					/* Echec de l'ouverture */
					printf("ERREUR : ouverture du fichier impossible\n");
					Emission("550 - Impossible d'ouvrir le fichier\n",client);
					return 0;
				}else{
		
					/* On recupere la taille du fichier */
					fseek (fichier , 0 , SEEK_END);
			  		taille = ftell (fichier);
			  		rewind (fichier);
			  		/* On envoi la taille du fihier */
			  		sprintf(reponse,"213 %ld\n",taille);
			  		Emission(reponse,client);
			  		printf("Taille du fichier envoyee\n");
			  		return 1;
				}
			}
		}
	}
}


/* Envoi une partie d'un fichier au client */
int envoyerPartieFichier(Client *client, char *requete){
	FILE * fichier; /* Fichier que l'on veut envoyer */
	char nomFichier[100]; /* Chemin du fichier que l'on veut envoyer */
	char fichierSave[100]; /* Sauvegarde du nom du fichier car il s'efface au cours de l'execution */
	char commande[5]; /* Commande de l'utilisateur */
	int debut; /* Fin de la partie */
	int fin; /* Debut de la partie */
	char *reponse; /* Reponse pour le client */
	char requeteSave[500]; /* Sauvegarde de la requete */
	char donnees[8191]; /* Donnees du bloc */
	char caractereCourrant; /* Caractere lu dans le fichier */

	strcpy(requeteSave,requete);

	/* on recupere les elements presents dans la requete */
	if(sscanf(requete,"%s %s %d %d",commande,nomFichier,&debut,&fin) < 1){
		/* Erreur dans lecture chaine */
		printf("Erreur lecture chaine\n");
		Emission("451 - ERREUR decomposition de la requete echouee\n",client);
		return 0;
	}else{
		/* on sauvegarde le nom du fichier */
		strcpy(fichierSave,nomFichier);
		/* On verifie que la commande est bien REST */
		if(strcmp(commande,"REST") != 0){
			/* On envoie l'erreur au client */
			printf("Requete incorrecte : mauvaise commande\n");
			Emission("500 - Requete incorrecte\n",client);
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
				/* On va donc comparer la requete sauvegardee a une requete que l'on monte pour le test */
				sprintf(requete,"REST %s %d %d\n",nomFichier,debut, fin);
				if(strlen(requeteSave) != strlen(requete)){
					/* requete incorrecte */
					printf("Requete incorrecte : probleme de longueur\n");
					Emission("500 - Requete incorrecte\n",client);
					return 0;
				}else{
					/* On recupere le nom du fichier de la sauvegarde */
					strcpy(nomFichier,fichierSave);
					/* On ouvre le fichier en mode binaire */
					fichier = NULL; /* on met fichier a NULL pour bien pouvoir tester l'ouverture */
					fichier = fopen(nomFichier,"rb");
					/* On teste l'ouverture du fichier */
					if(fichier == NULL){
						/* Echec de l'ouverture */
						printf("ERREUR : ouverture du fichier impossible\n");
						Emission("550 - Impossible d'ouvrir le fichier\n",client);
						return 0;
					}else{
						int nbCaractereLus; /* compteur du nombre total de caracteres lus */
						int compteur; /* compteur pour le bloc */
						/* On informe le client que le telechargement va commencer */
						Emission("150 - Debut du telechargement\n",client);
						/* On positionne le compteur de caractere a 0 */
						compteur = 0;
						nbCaractereLus = 0;
						/* On se positionne au bon endroit dans le fichier */
						fseek(fichier,debut,SEEK_SET);
						/* On va lire le fichier caractere par caractere */
						do{
							/* On recupere le premier caractere */
							caractereCourrant = fgetc(fichier);
							/* Si on a atteint la fin du fichier, on prepare le bloc et on envoie */
							if(nbCaractereLus == fin){
								unsigned char descripteur = 0;
								EmissionBinaire(&descripteur,1,client);
								unsigned short taille = htons(compteur); /* ntohs cote reception */
								EmissionBinaire((char*)&taille,2,client);
								EmissionBinaire(donnees,compteur,client);
								reponse = Reception(client);
								if(strstr(reponse,"OK") == NULL){
									printf("Erreur client\n");
									return 0;	
								}
							}else{
								/* On ajoute le caractere a la partie des donnees du bloc */
								donnees[compteur] = caractereCourrant;
								/* On incremente le compteur */
								compteur++;
								nbCaractereLus++;
								/* Si le compteur atteint la taille du bloc, on prepare le bloc et on envoie le bloc */
								if(compteur == 8191){
									unsigned char descripteur = 0;
									EmissionBinaire(&descripteur,1,client);
									unsigned short taille = htons(8191); /* ntohs cote reception */
									EmissionBinaire((char*)&taille,2,client);
									EmissionBinaire(donnees,8191,client);
									reponse = Reception(client);
									if(strstr(reponse,"OK") == NULL){
										printf("Erreur client\n");
										return 0;	
									}
									/* On remet le compteur a 0 */
									compteur = 0;
								}
							}
						}while(caractereCourrant != EOF);
						unsigned char descripteur = 64;
						EmissionBinaire(&descripteur,1,client);
						unsigned short taille = 0; /* ntohs cote reception */
						EmissionBinaire((char*)&taille,2,client);
						/* On teste le retour client et on envoie le message correspondant */
						if(strstr(Reception(client),"OK") != NULL){
							/* Le retour client contient bien OK */
							printf("Telechargement OK\n");
							Emission("226 - Telechargement termine\n",client);
						}else{
							printf("Telechargement KO\n");
							Emission("451 - Telechargement echoue\n",client);
						}
						/* On quitte la fonction avec le code retour 1 */
						return 1;
					}
				}
			}
		}
	}
}

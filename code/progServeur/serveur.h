#ifndef __SERVEUR_H__
#define __SERVEUR_H__

#define LONGUEUR_TAMPON 4096

typedef struct {
	/* le socket de service */
	int socketService;
	/* le tampon de reception */
	char tamponClient[LONGUEUR_TAMPON];
	int debutTampon;
	int finTampon;
} Client;


/* Initialisation.
 * Creation du serveur en précisant le service ou numéro de port.
 * renvoie 1 si ça c'est bien passŽ 0 sinon
 */
int InitialisationAvecService(char *service);


/* Attends qu'un client se connecte.
 * renvoie un pointeur vers une nouvelle structure Client  si ça c'est bien passé NULL sinon
 */
Client *AttenteClient();

/* Recoit un message envoye par le client.
 * retourne le message ou NULL en cas d'erreur.
 * Note : il faut liberer la memoire apres traitement.
 */
char *Reception(Client *client);

/* Envoie un message au client.
 * Attention, le message doit etre termine par \n
 * renvoie 1 si a c'est bien passŽ 0 sinon
 */
int Emission(char *message, Client *client);

/* Ferme la connexion avec le client.
 */
void TerminaisonClient(Client *client);

/* Arrete le serveur.
 */
void Terminaison();

/* Met tous les caractères d'une chaîne en majuscule */
char * putMajuscule(char *ch);

/* Liste le répertoire passé en paramètre */
char * listeDir(char *dir);

/* Réalise la connexion du client en paramètre sur le serveur FTP 
retourne 1 si client connecte et 0 sinon*/
int connecterClient(Client *client);

/* Permet au client d'envoyer un fichier sur le serveur, si le fichier est déjà présent sur le serveur on écrase */
void recevoirFichier(Client *client, char *requete);

/* Permet au serveur d'envoyer un fichier à un client qui en fait la demande */
int envoyerFichier(Client *client, char *requete);

#endif

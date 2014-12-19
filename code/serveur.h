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
 * Creation du serveur en pr�cisant le service ou num�ro de port.
 * renvoie 1 si �a c'est bien pass� 0 sinon
 */
int InitialisationAvecService(char *service);


/* Attends qu'un client se connecte.
 * renvoie un pointeur vers une nouvelle structure Client  si �a c'est bien pass� NULL sinon
 */
Client *AttenteClient();

/* Recoit un message envoye par le client.
 * retourne le message ou NULL en cas d'erreur.
 * Note : il faut liberer la memoire apres traitement.
 */
char *Reception(Client *client);

/* Envoie un message au client.
 * Attention, le message doit etre termine par \n
 * renvoie 1 si �a c'est bien pass� 0 sinon
 */
int Emission(char *message, Client *client);

/* Ferme la connexion avec le client.
 */
void TerminaisonClient(Client *client);

/* Arrete le serveur.
 */
void Terminaison();

/* Met tous les caract�res d'une cha�ne en majuscule */
char * putMajuscule(char *ch);

/* Liste le r�pertoire pass� en param�tre */
char * listeDir(char *dir);

/* R�alise la connexion du client en param�tre sur le serveur FTP 
retourne 1 si client connecte et 0 sinon*/
int connecterClient(Client *client);

/* Permet au client d'envoyer un fichier sur le serveur, si le fichier est d�j� pr�sent sur le serveur on �crase */
void recevoirFichier(Client *client, char *requete);

#endif

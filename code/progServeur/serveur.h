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
 * Creation du serveur en precisant le service ou numero de port.
 * renvoie 1 si ca c'est bien passe 0 sinon
 */
int InitialisationAvecService(char *service);


/* Attends qu'un client se connecte.
 * renvoie un pointeur vers une nouvelle structure Client si ca c'est bien passe NULL sinon
 */
Client *AttenteClient();

/* Recoit un message envoye par le client.
 * retourne le message ou NULL en cas d'erreur.
 * Note : il faut liberer la memoire apres traitement.
 */
char *Reception(Client *client);

/* Envoie un message au client.
 * Attention, le message doit etre termine par \n
 * renvoie 1 si ca c'est bien passe 0 sinon
 */
int Emission(char *message, Client *client);

/* Envoie des donnees au client en precisant leur taille.
 */
int EmissionBinaire(char *donnees, size_t taille, Client *client);

/* Ferme la connexion avec le client.
 */
void TerminaisonClient(Client *client);

/* Arrete le serveur.
 */
void Terminaison();

/* Met tous les caracteres d'une chaine en majuscule */
char * putMajuscule(char *ch);

/* Liste le repertoire passe en parametre */
char * listeDir(char *dir);

/*
Parametres : str : chaine principale / len : longueur de la sous-chaine / pos : debut de la sous-chaine  
Extrait la sous-chaine de longueur "len" à partir du caractere numero "pos" dans la chaine "str" 
*/
char *extraireSousChaine(char *str, long len, long pos);

/* Realise la connexion du client en parametre sur le serveur FTP 
retourne 1 si client connecte et 0 sinon*/
int connecterClient(Client *client);

/* Permet au client d'envoyer un fichier sur le serveur, si le fichier est deja present sur le serveur on ecrase */
void recevoirFichier(Client *client, char *requete);

/* Permet au serveur d'envoyer un fichier a un client qui en fait la demande */
int envoyerFichier(Client *client, char *requete);

/* Envoi en mode bloc, retourne 1 si ok et 0 sinon */
int envoyerFichierBloc(Client *client, char *requete);

/* Change le mode de transfert des fichier, retourne NULL si KO ou le codeMode si OK */
char changerMode(char *requete, Client *client);

/* Renvoi au client la taille du fichier qu'il donne en parametre */
int tailleFichier(char *requete, Client *client);

/* Envoi une partie d'un fichier au client */
int envoyerPartieFichier(Client *client, char *requete);

#endif

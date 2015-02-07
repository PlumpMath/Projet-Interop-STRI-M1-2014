#ifndef __P2P_H__
#define __P2P_H__

#define LONGUEUR_TAMPON 4096

typedef struct {
	/* le socket de service */
	int socketService;
	/* le tampon de reception */
	char tamponClient[LONGUEUR_TAMPON];
	int debutTampon;
	int finTampon;
} Client;

/* Strucutre de donnees pour les thread serveur */
typedef struct{
	char portEcoute[6];
}donneesServeur;

/* Strucutre de donnees pour les thread de telechargement */
typedef struct{
	/* data */
	char numPort[10];
	char nomFichier[100];
	int numeroServeur;
	int nombreServeurs;
}donneesThread;


/* Initialisation.
 * Connexion au serveur sur la machine donnee et au service donne.
 * Utiliser localhost pour un fonctionnement local.
 */
int InitialisationAvecServiceModeClient(char *machine, char *service);

/* Recoit un message envoye par le serveur.
 */
char *ReceptionModeClient();

/* Envoi un message au serveur.
 */
int EmissionModeClient(char *message);

/* Recoit des donnees envoyees par le serveur.
 */
int ReceptionBinaireModeClient(char *donnees, size_t tailleMax);

/* Ferme la connexion.
 */
void TerminaisonModeClient();

/* 
Connexion au serveur FTP.
retourne 1 si la connexion est OK et 0 sinon
*/
int connecterUtilisateurModeClient();

/* Envoi un fichier present dans le dossier courant sur le serveur */
void envoyerFichierModeClient(char *nomFichier);

/* Telecharge un fichier depuis le serveur */
void telechargerFichierModeClient(char *nomFichier);

/* Telecharge un fichier depuis le serveur */
int telechargerFichierBlocModeClient(char *nomFichier);

/* Envoi au serveur une demande de changement du mode de telechargement des fichiers */
void changerModeModeClient(char mode);

/* Permet de reprendre un téléchargement en cours en cas d'erreur */
int repriseTelechargementModeClient(char *nomFichier);

/* Thread pour le téléchargement parallèle de fichiers en mode bloc */
void *telechargerFichierBlocThreadModeClient(void* param);

/* Thread pour l'execution de la partie "client" du programme */
void *threadModeClient(void* param);

/* Initialisation.
 * Creation du serveur en precisant le service ou numero de port.
 * renvoie 1 si ca c'est bien passe 0 sinon
 */
int InitialisationAvecServiceModeServeur(char *service);

/* Attends qu'un client se connecte, quand un client se connecte on lance un processus fils pour le traitement de sa requete */
Client * AttenteClientModeServeur();

/* Recoit un message envoye par le serveur.
 */
char *ReceptionModeServeur(Client *client);

/* Envoie un message au client.
 */
int EmissionModeServeur(char *message, Client *client);

/* Envoie des donnes au client en prcisant leur taille.
 */
int EmissionBinaireModeServeur(char *donnees, size_t taille, Client *client);

/* Ferme la connexion avec le client.
 */
void TerminaisonClientModeServeur(Client *client);

/* Arrete le serveur.
 */
void TerminaisonModeServeur();

/* Met tous les caracteres d'une chaine en majuscule */
char * putMajusculeModeServeur(char *ch);

/*
Paramètres : str : chaine principale / len : longueur de la sous-chaine / pos : début de la sous-chaine  
Extrait la sous-chaine de longueur "len" à partir du carcatère numéro "pos" dans la chaine "str" 
*/
char *extraireSousChaineModeServeur(char *str, long len, long pos);

/* Realise la connexion du client en parametre sur le serveur FTP 
retourne 1 si client connecte et 0 sinon*/
int connecterClientModeServeur(Client *client);

/* Permet au client d'envoyer un fichier sur le serveur, si le fichier est deja present sur le serveur on ecrase */
void recevoirFichierModeServeur(Client *client, char *requete);

/* Permet au serveur d'envoyer un fichier a un client qui en fait la demande */
int envoyerFichierModeServeur(Client *client, char *requete);

/* Envoi en mode bloc, retourne 1 si ok et 0 sinon */
int envoyerFichierBlocModeServeur(Client *client, char *requete);

/* Change le mode de transfert des fichier, retourne NULL si KO ou le codeMode si OK */
char changerModeModeServeur(char *requete, Client *client);

/* Renvoi au client la taille du fichier qu'il donne en paramètre */
int tailleFichierModeServeur(char *requete, Client *client);

/* Envoi une partie d'un fichier au client */
int envoyerPartieFichierModeServeur(Client *client, char *requete);

/* Thread pour l'execution de la partie serveur */
void *threadModeServeur(void* param);


#endif

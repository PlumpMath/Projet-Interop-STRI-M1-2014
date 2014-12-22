#ifndef __CLIENT_H__
#define __CLIENT_H__

/* Initialisation.
 * Connexion au serveur sur la machine donnee et au service donne.
 * Utilisez localhost pour un fonctionnement local.
 * renvoie 1 si Áça c'est bien passe 0 sinon
 */
int InitialisationAvecService(char *machine, char *service);

/* Recoit un message envoye par le serveur.
 * retourne le message ou NULL en cas d'erreur.
 * Note : il faut liberer la memoire apres traitement.
 */
char *Reception();

/* Envoie un message au serveur.
 * Attention, le message doit etre termine par \n
 * renvoie 1 si ça c'est bien passe 0 sinon
 */
int Emission(char *message);

/* Recoit des donnees envoyees par le serveur.
 * renvoie le nombre d'octets reçus, 0 si la connexion est fermee,
 * un nombre negatif en cas d'erreur
 */
int ReceptionBinaire(char *donnees, size_t tailleMax);

/* Envoie des donnees au serveur en precisant leur taille.
 * renvoie le nombre d'octets envoyes, 0 si la connexion est fermee,
 * un nombre negatif en cas d'erreur
 */
int EmissionBinaire(char *donnees, size_t taille);

/* Ferme la connexion.
 */
void Terminaison();

/* Recupere la liste des fichiers dans le repertoire courant du serveur */
void listeFichiers();

/* 
Connexion au serveur FTP.
retourne 1 si la connexion est OK et 0 sinon
*/
int connecterUtilisateur();

/* Envoi un fichier present dans le dossier courant sur le serveur */
void envoyerFichier(char *nomFichier);

/* Telecharge un fichier depuis le serveur */
void telechargerFichier(char *nomFichier);

#endif

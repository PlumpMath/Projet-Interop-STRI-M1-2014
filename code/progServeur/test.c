#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/*
Paramètres : str : chaine principale / len : longueur de la sous-chaine / pos : début de la sous-chaine  
Extrait la sous-chaine de longueur "len" à partir du carcatère numéro "pos" dans la chaine "str" 
*/
char *extraireSousChaine(char *str, long len, long pos){
	long i; /* indice de parcours de la chaine */
	char sousChaine[len]; /* Sous chaine que l'on va retourner */

	/* On se positionne sur le début de la sous chaine et on récupère les i caractères */
	for(i=pos;i<(pos+len);i++){
		sousChaine[i-pos] = str[i];
	}

	/* On retourne la sous chaine */
	return sousChaine;
}



int main(){
	int i = 896;
	printf("%04d",i);

	return 0;
}

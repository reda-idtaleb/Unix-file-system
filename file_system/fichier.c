#include "disk.h"
#include "volume.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#define NB_BLOC_DIRECT 8

/* #### LES STRUCTURES DE DONNEES #### */
enum file_type_e {
	FILE_TYPE,
	DIRECTORY_TYPE
};

struct inode_s_memory {
	unsigned int inumero;				// numéro d'un inoeud = numéro de son bloc (4 octets)
	enum file_type_e type_file;			// type(file, repertoire) (4 octets)
	int taille;							// taille d'un inoeud en octets (4 octets)
	unsigned char *payload;		        // contenu du fichier (8 octets)
	unsigned char padding[BLOCKSIZE - 20];	  
};

struct inode_s_disk {
	unsigned int bloc_numero; // (4 octets)
	enum file_type_e type_file; // (4 octets)
	int taille;	// (4 octets)
	unsigned bloc_direct[NB_BLOC_DIRECT]; // tableau des 8 premiers blocs du fichier (4 * 8 octets)
	unsigned bloc_indirect;				  // blocs d'adresses, init à 0 // (4 octets)
	unsigned bloc_double_indirect;		  // init à 0 // (4 octets)
	unsigned char padding[BLOCKSIZE - 52];	  
};

/* #### DECLARATION DE FONCTIONS #### */
void read_inode(unsigned int inumber, struct inode_s_memory *inode);
void write_inode(unsigned int inumber, struct inode_s_memory *inode);
unsigned int create_inode(enum file_type_e type);
int delete_inode(unsigned int inumber);
unsigned int copy_payload_to_bloc(unsigned int *bloc, struct inode_s_memory *inode, unsigned int n, int *offset);
void init_elements_bloc(unsigned int *elements, unsigned int elements_nb);
void copy_bloc_to_payload(unsigned int *bloc, struct inode_s_memory *inode, unsigned int n, int *offset);
void test_couche3(unsigned char * content);


/* #### DEFINITION DE FONCTIONS #### */
void read_inode(unsigned int inumber, struct inode_s_memory *inode) {
	int offset = 0;
	struct inode_s_disk inodeDisk;

	read_bloc(PARTITION_VOL, inumber, (unsigned char *) &inodeDisk);

	inode->inumero = inumber;
	inode->type_file = inodeDisk.type_file;
	inode->taille = inodeDisk.taille;
	inode->payload = (unsigned char * )malloc(inode->taille * sizeof(unsigned char));
	
	// On boucle sur les blocs directs

	copy_bloc_to_payload(inodeDisk.bloc_direct, inode, NB_BLOC_DIRECT, &offset);
	
	// Si on a des blocs indirects occupés
	if (inodeDisk.bloc_indirect != 0){
		unsigned char * block_indirect;
		block_indirect = malloc(BLOCKSIZE * sizeof(unsigned char));
		read_bloc(PARTITION_VOL, inodeDisk.bloc_indirect, block_indirect);
		copy_bloc_to_payload((unsigned int *) block_indirect, inode, BLOCKSIZE, &offset);
	}
	// Si on a des blocs doublement indirects occupés
	if (inodeDisk.bloc_double_indirect != 0){
		unsigned char *bloc_double_indirect;
		bloc_double_indirect = malloc(BLOCKSIZE * sizeof(unsigned char));
		read_bloc(PARTITION_VOL, inodeDisk.bloc_double_indirect, bloc_double_indirect);
		for (int i = 0; i < BLOCKSIZE; i++){
			if (bloc_double_indirect[i] == 0){
				continue;
			}
			else{
				unsigned char *sous_bloc_double_indirect;
				sous_bloc_double_indirect = malloc(BLOCKSIZE * sizeof(unsigned char));
				read_bloc(PARTITION_VOL, bloc_double_indirect[i], sous_bloc_double_indirect);
				copy_bloc_to_payload((unsigned int *) sous_bloc_double_indirect, inode, BLOCKSIZE, &offset);
			}
		}
	}
}

void write_inode(unsigned int inumber, struct inode_s_memory *inode) {
	int offset = 0;
	unsigned int memoryLeft = 0;
	struct inode_s_disk inodeDisk;
	inodeDisk.bloc_numero = inumber;
	inodeDisk.type_file = inode->type_file;
	inodeDisk.taille = inode->taille;
	// Les numéros de blocs directes sont initialisés à 0 par défaut
	init_elements_bloc(inodeDisk.bloc_direct, NB_BLOC_DIRECT);
	
	memoryLeft = copy_payload_to_bloc(inodeDisk.bloc_direct, inode, NB_BLOC_DIRECT, &offset);
	// si les blocs directes ne sont pas suffisants
	if (memoryLeft > 0){
		int newBloc1;
        if ((newBloc1 = new_bloc()) == 0) {
            printf("Aucun bloc disponible sur le disque!\n");
            exit(EXIT_FAILURE);
        }
		
		unsigned char *bloc_indirect;
		bloc_indirect = malloc(BLOCKSIZE * sizeof(unsigned char));
		// tous les numéros des blocs contenus dans le bloc indirecte sont initalisé à 0
		init_elements_bloc((unsigned int *) bloc_indirect, BLOCKSIZE);
		memoryLeft = copy_payload_to_bloc((unsigned int *) bloc_indirect, inode, BLOCKSIZE, &offset);
		write_bloc(PARTITION_VOL, newBloc1, bloc_indirect);
		free(bloc_indirect);

		// Si le blocs indirect n'a pas été suffisant, on crée le bloc double indirect
		if (memoryLeft > 0){
			int newBloc2;
			if ((newBloc2 = new_bloc()) == 0) {
				printf("Aucun bloc disponible sur le disque!\n");
				exit(EXIT_FAILURE);
			}
			unsigned char * bloc_double_indirect;
			bloc_double_indirect = malloc(BLOCKSIZE * sizeof(unsigned char));
			init_elements_bloc((unsigned int *) bloc_double_indirect, BLOCKSIZE);
			// On crée les sous blocs double indirect
			for (int i = 0; i < BLOCKSIZE; i++){
				if (memoryLeft > 0){
					int newBloc3;
					if ((newBloc3 = new_bloc()) == 0) {
						printf("Aucun bloc disponible sur le disque!\n");
						exit(EXIT_FAILURE);
					}
					unsigned char * sous_bloc_double_indirect;
					sous_bloc_double_indirect = malloc(BLOCKSIZE * sizeof(unsigned char));
					init_elements_bloc((unsigned int *) sous_bloc_double_indirect, BLOCKSIZE);
					memoryLeft = copy_payload_to_bloc((unsigned int * )sous_bloc_double_indirect, inode, BLOCKSIZE, &offset);
					
					write_bloc(PARTITION_VOL, newBloc3, sous_bloc_double_indirect);
					free(sous_bloc_double_indirect);
				}
				else{
					break;
				}
			}
			write_bloc(PARTITION_VOL, newBloc2, bloc_double_indirect);
			free(bloc_double_indirect);
		}
	}
	// si les blocs directes ont été suffisants
	else {
		inodeDisk.bloc_indirect = 0;
		inodeDisk.bloc_double_indirect = 0;
	}
	write_bloc(PARTITION_VOL, inumber, (unsigned char *) &inodeDisk);

}

// Cette fonction copie le contenu du payload dans des blocs et ajoute les numéros des blocs crée au bloc parent.
// Elle retourne la nombre d'octets qui reste à écrire
unsigned int copy_payload_to_bloc(unsigned int *bloc, struct inode_s_memory *inode, unsigned int n, int *offset){
	if (inode->taille - *offset <= 0){
		return 0;
	}
	unsigned int memoryLeft;
	for (unsigned int i = 0; i < n; i++){
		memoryLeft = inode->taille - *offset;
		if (memoryLeft == 0){
			return memoryLeft;
		}
		unsigned char buffer[BLOCKSIZE];
		int newBloc;
		printf("memory left : %d\n", memoryLeft);

		if ((newBloc = new_bloc()) == 0) {
			printf("Aucun bloc disponible sur le disque!\n");
			exit(EXIT_FAILURE);
		}
		// si payload contient encore au moins 256 octets 
		if (memoryLeft >= BLOCKSIZE){
			memcpy(buffer, inode->payload+ *offset, BLOCKSIZE);
			write_bloc(PARTITION_VOL, newBloc, buffer);
			*offset += BLOCKSIZE;
			bloc[i] = newBloc;
		}
		// si payload contient moins de 256 octets 
		else {
			memcpy(buffer, inode->payload+ *offset, memoryLeft);
			write_bloc(PARTITION_VOL, newBloc, buffer);
			*offset += memoryLeft; 
			bloc[i] = newBloc;
			memoryLeft = inode->taille - *offset;
			break;
		}
	}
	return memoryLeft;
}

// Cette fonction initialise les numéros de blocs à 0
void init_elements_bloc(unsigned int *elements, unsigned int elements_nb){
	for (unsigned int i = 0; i < elements_nb; i++)
		elements[i] = 0;
}

// Cette fonction copie le contenu d'un bloc dans le payload
void copy_bloc_to_payload(unsigned int *bloc, struct inode_s_memory *inode, unsigned int n, int *offset) {
	for (unsigned int i = 0; i < n; i++) {
		if (bloc[i] == 0)
			continue;
		else {
			unsigned char *buffer;
			buffer = malloc(BLOCKSIZE * sizeof(unsigned char));
			read_bloc(PARTITION_VOL, bloc[i], buffer);
			unsigned int memoryLeft = inode->taille - *offset;
			if (memoryLeft >= BLOCKSIZE){
				memcpy(inode->payload+ *offset, buffer, BLOCKSIZE);
				*offset += BLOCKSIZE; 
			}
			else {
				memcpy(inode->payload+ *offset, buffer, memoryLeft);
				*offset += memoryLeft; 
			}
		}
	}
}

unsigned int create_inode(enum file_type_e type) {
	int newBloc = 0;
    if ((newBloc = new_bloc()) == 0) {
        printf("Aucun bloc disponible sur le disque!\n");
        return newBloc;
    }
	struct inode_s_memory inode;
	inode.inumero = newBloc;
	inode.type_file = type;
	inode.taille = 0;
	inode.payload = malloc(sizeof(unsigned char));
	write_inode(newBloc, &inode);
	return newBloc;
}

int delete_inode(unsigned int inumber) {
	struct inode_s_disk inodeDisk;
	read_bloc(PARTITION_VOL, inumber, (unsigned char *) &inodeDisk);
	for (int i = 0; i < NB_BLOC_DIRECT; i++){
		if (inodeDisk.bloc_direct[i] != 0){
			free_bloc(inodeDisk.bloc_direct[i]);
		}
	}
	if (inodeDisk.bloc_indirect != 0){
		unsigned char * bloc_indirect;
		bloc_indirect = malloc(BLOCKSIZE * sizeof(unsigned char));
		read_bloc(PARTITION_VOL, inumber, bloc_indirect);
		for (int i = 0; i < BLOCKSIZE; i++){
			if (bloc_indirect[i] != 0){
				free_bloc(bloc_indirect[i]);
			}
		}
		free_bloc(inodeDisk.bloc_indirect);
		free(bloc_indirect);
	}

	if (inodeDisk.bloc_double_indirect != 0){
		unsigned char *bloc_double_indirect;
		bloc_double_indirect = malloc(BLOCKSIZE * sizeof(unsigned char));
		read_bloc(PARTITION_VOL, inumber, bloc_double_indirect);
		for (int i = 0; i < BLOCKSIZE; i++){
			if (bloc_double_indirect[i] != 0){
				unsigned char *sous_bloc_double_indirect;
				sous_bloc_double_indirect = malloc(BLOCKSIZE * sizeof(unsigned char));
				read_bloc(PARTITION_VOL, inumber, sous_bloc_double_indirect);
				for(int j=0; j<BLOCKSIZE; j++){
					if (sous_bloc_double_indirect[j] != 0){
						free_bloc(sous_bloc_double_indirect[j]);
					}
				}
				free_bloc(bloc_double_indirect[i]);
				free(sous_bloc_double_indirect);
			}
		}
		free_bloc(inodeDisk.bloc_double_indirect);
		free(bloc_double_indirect);
	}
	free_bloc(inumber);
	return inumber;
}

void test_couche3(unsigned char * content){
	unsigned int nbloc = create_inode(FILE_TYPE);
	if (nbloc == 0){
		printf("disque plein");
		exit(EXIT_FAILURE);
	}
	struct inode_s_memory inode;
	read_inode(nbloc, &inode);
	printf("\n");
	printf("Vérification de la création de l'inode\n\n");
	printf("type : %d\n", inode.type_file);
	printf("taille : %d\n", inode.taille);
	printf("inumero : %d\n", inode.inumero);
	printf("payload : ");
	dump(inode.payload, sizeof(unsigned char));
	printf("\n\n");

	inode.payload = content;
	inode.taille = strlen((char *)content);	
	write_inode(nbloc, &inode); 

	struct inode_s_memory inode1;
	read_inode(nbloc, &inode1);

	printf("Lecture de l'inode écrit dans le disque\n\n");
	printf("type : %d\n", inode1.type_file);
	printf("taille : %d\n", inode1.taille);
	printf("inumero : %d\n", inode1.inumero);
	printf("payload : ");
	dump(inode1.payload, inode1.taille*sizeof(unsigned char));
	printf("\n");

	printf("Vérification du nombre de blocs libres : %d \n", superBlock.number_of_free_blocks);
	printf("\n");
	printf("suppression de l'inode\n");
	delete_inode(nbloc);
	printf("Vérification du nombre de blocs libres : %d \n", superBlock.number_of_free_blocks);
}

int main() {
	init_volume(PARTITION_VOL);
	printf("########### TEST AVEC UN FICHIER DE 11 OCTETS ###########\n\n");
	char *content1 = malloc(12 * sizeof(char));
	content1 = "hello world";
	test_couche3((unsigned char *) content1);

	printf("########### TEST AVEC UN FICHIER DE 2304 OCTETS ###########\n\n");
	init_volume(PARTITION_VOL);

	unsigned char * content2 = malloc(2304 * sizeof(unsigned char));
	for (int i = 0; i<2304; i++){
		content2[i] = 'a';
	}
	dump(content2, 2304);
	test_couche3(content2);

	exit(EXIT_SUCCESS);

}


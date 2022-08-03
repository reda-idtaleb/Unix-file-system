#include "disk.h"
#include "volume.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>

void dump(unsigned char *buffer, unsigned size) {
    unsigned i;
    for (i = 0; i < size; i++) {
        printf("%02x ", buffer[i]);
        if(i % 16 == 15)
            printf("\n");
        else if(i % 16 == 7)
            printf("  ");
    }
}

void init_volume(unsigned int vol) {
	char name[] = "superBlock";
	init_disk();
	format_vol(vol);

	superBlock.magic = MAGIC_NUMBER;
	superBlock.serialNumber = PARTITION_VOL;
	strcpy(superBlock.name, name);
	superBlock.first_free_block = 1;
	superBlock.number_of_free_blocks = NBBLOCKS - 1;

	save_super();
	
	// chaînage des blocks libres
	struct free_bloc free_block;
	for (int i = 1; i < NBBLOCKS; i++) { 
		free_block.numero_block = i;
		free_block.next_block = (i+1)%NBBLOCKS;
		write_bloc(vol, free_block.numero_block, (unsigned char *) &free_block);
	}
}

int load_super(unsigned int vol) {
	read_bloc(vol, superBlock.serialNumber, (unsigned char *) &superBlock);
	return 1;
}

void save_super() {
	if (superBlock.magic != MAGIC_NUMBER) {
		printf("super block n'est pas initialisé\n");
		exit(EXIT_FAILURE);
	}
	write_bloc(0, superBlock.serialNumber, (unsigned char *) &superBlock);
}

unsigned int new_bloc() {
	// on vérifie si le super block est initialisé
	if (superBlock.magic != MAGIC_NUMBER) {
		printf("Super block n'est pas initialisé.\n");
		return 0;
	}

	if (superBlock.number_of_free_blocks == 0) {
		printf("Disque plein! Aucun bloc n'est libre.\n");
		return 0;
	}
	
	struct free_bloc free_block;
	read_bloc(0, superBlock.first_free_block, (unsigned char*) &free_block); 

	int new_free_block = superBlock.first_free_block;
	superBlock.first_free_block = free_block.next_block;
	superBlock.number_of_free_blocks--;
	save_super();
	return new_free_block;
}

void free_bloc(unsigned int bloc) {
	if (bloc == 0) {
		printf("Le super block ne peut pas être libéré.\n");
		return; 
	}

	if (superBlock.magic != MAGIC_NUMBER) {
		printf("Super block n'est pas initialisé.\n");
		return;
	}

	struct free_bloc free_block;
	// le bloc à libérer va pointer vers le 1er bloc libre auquel pointe le super block
	free_block.next_block = superBlock.first_free_block;
	free_block.numero_block = bloc; // le numéro du bloc libéré est celui passé en argument
	write_bloc(PARTITION_VOL, bloc, (unsigned char*) &free_block);
	
	// Le bloc libéré se place au début de la file(juste après superblock)
	superBlock.first_free_block = bloc;
	superBlock.number_of_free_blocks++; // on incrémente le nbre de block libre
	save_super();
}

int main_vol(){
	init_volume(PARTITION_VOL);

	printf("\n* Fait appel à la fonction new_bloc() jusqu’à ce qu’elle retourne une erreur\n");
	int bloc;
	while ((bloc = new_bloc()) != 0)
		printf(">> new bloc n°%d alloué.\n", bloc);
	
	printf("\n* Vérifie que le disque est plein\n");
	if (superBlock.number_of_free_blocks != 0)
		printf(">> Disque non plein.\n");
	else 
		printf(">> Disque plein.\n");

	printf("\n* Itère un nombre aléatoire de fois sur la libération d’un bloc free_bloc()\n");
	// on itère sur nombre de blocks aléatoire
	int random_iter = rand() % (NBBLOCKS +1); 
	int i = 1;
	printf(">> Nombre d'itération: %d\n", random_iter);
	while(i < random_iter) {
		free_bloc(i);
		i++;
	}

	load_super(0);
	printf("\n* Affiche le statut du disque (taille libre)\n");
	printf(">> Statut disque - taille libre: %d\n", superBlock.number_of_free_blocks);

	printf("\n* Alloue des blocs tant que le disque est non plein\n");
	i = 0;
	while (superBlock.number_of_free_blocks > 0) {
		bloc = new_bloc();
    	i++;
	}
	printf(">> Nombre de blocs alloués est %d.\n", i);
	printf("\n* Terminé.\n");
	return 0;

}

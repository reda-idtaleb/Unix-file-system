#define MAGIC_NUMBER 0x12345678
#define PARTITION_VOL 0

void init_volume(unsigned int vol);
int  load_super(unsigned int vol);
void save_super();

void dump(unsigned char *buffer, unsigned size);

unsigned new_bloc();
void free_bloc(unsigned int bloc);

struct superBlock_s {
	int magic;
	int serialNumber; // unique identifier for the volume
	char name[32];	 
	int first_free_block; // head of the list of free blocks	
	int number_of_free_blocks;
	char padding[204];
} superBlock;

struct free_bloc {
	int numero_block;
	int next_block;
	char padding[248];
};
#ifndef _DISK_H_
#define _DISK_H_

#include <stdio.h>
#include <stdlib.h>

/* Configuration */

#define DISKFILE "disk.img"
#define NBBLOCKS  256
#define BLOCKSIZE 256


/* API */

extern void init_disk(void);
extern void read_bloc(unsigned int vol, unsigned int nbloc, unsigned char *buffer);
extern void write_bloc(unsigned int vol, unsigned int nbloc, const unsigned char *buffer);
extern void format_vol(unsigned int vol);


/* Macros for errors and warnings */

#define err(msg) { printf("Error in %s: %s\n", __func__, msg); abort(); }
#define warn(msg) { printf("Warning in %s: %s\n", __func__, msg); }

#endif

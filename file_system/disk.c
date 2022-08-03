#include "disk.h"
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

static int disk_fd = -1;
static unsigned int seed = 0;

void init_disk(void) {
    seed = time(NULL);
    disk_fd = open(DISKFILE, O_RDWR|O_CREAT, 0644);
    if(disk_fd == -1) {
        perror("impossible to open disk file");
        abort();
    }
}

void random_fill(unsigned char *buffer, unsigned count) {
    unsigned i;
    for (i = 0; i < count; i++)
        buffer[i] = rand_r(&seed) & 0xff;
}

void read_bloc(unsigned int vol, unsigned int nbloc, unsigned char *buffer) {
    int r;
    if(vol != 0) err("only volume 0 is allowed");
    if(disk_fd < 0) {
        err("disk not initialized");
    }
    if(nbloc >= NBBLOCKS) {
        warn("overread, filling with random bytes");
        random_fill(buffer, BLOCKSIZE);
        return;
    }

    r = pread(disk_fd, buffer, BLOCKSIZE, nbloc*BLOCKSIZE);
    if(r == -1) {
        perror("impossible to read disk file");
        abort();
    }
    if(r < BLOCKSIZE) {
        if (r) {
            warn("partial block read (corrupted disk image?), filling with random bytes");
        } else {
            warn("no data read (unformated disk?), filling with random bytes");
        }
        random_fill(buffer+r, BLOCKSIZE-r);
    }
}

void write_bloc(unsigned int vol, unsigned int nbloc, const unsigned char *buffer) {
    int w;
    if(vol != 0) err("only volume 0 is allowed");
    if(disk_fd < 0) {
        err("disk not initialized");
    }
    if(nbloc >= NBBLOCKS) {
        err("overwrite");
        return;
    }

    w = pwrite(disk_fd, buffer, BLOCKSIZE, nbloc*BLOCKSIZE);
    if (w == -1) {
        perror("impossible to write disk file");
        abort();
    }
    if (w < BLOCKSIZE) {
        err("partial block write");
    }
}

void format_vol(unsigned int vol) {
    unsigned i;
    unsigned char buffer[BLOCKSIZE];

    if(vol != 0) err("only volume 0 is allowed");
    memset(buffer, 0, BLOCKSIZE);
    for (i = 0; i < NBBLOCKS; i++)
        write_bloc(vol, i, buffer);
}

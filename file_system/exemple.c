#include <stdio.h>
#include "disk.h"

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

int main() {
    unsigned char buffer[BLOCKSIZE];
    unsigned char buffer2[BLOCKSIZE];

    init_disk();
    read_bloc(0, 15, buffer);
    dump(buffer, BLOCKSIZE);

    printf("Now formatting\n");
    format_vol(0);
    read_bloc(0, 15, buffer2);
    dump(buffer2, BLOCKSIZE);

    printf("Writing back initial content\n");
    write_bloc(0, 15, buffer);

    printf("Writing out of bounds\n");
    write_bloc(0, 256, buffer);

    return 0;
}

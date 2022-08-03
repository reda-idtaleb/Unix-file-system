CC := gcc
CFLAGS := -Wall -Wextra -g


exemple: disk.o exemple.o
	$(CC) $^ -o $@

volume: disk.o volume.o
	$(CC) $^ -o $@

fichier: disk.o volume.o fichier.o
	$(CC) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o exemple disk.img
	rm -f volume
	rm -f fichier

.PHONY: clean

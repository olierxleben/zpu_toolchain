
SRC=src/zpuio.c src/zpuload.c
OBJ=src/zpuio.o src/zpuload.o

CFLAGS=-O3 -Wall

all: $(OBJ)
	$(CC) $(CFLAGS) -lzpu -o zpuload src/zpuload.o
	$(CC) $(CFLAGS) -lzpu -o zpuio src/zpuio.o
	
clean:
	rm -f $(OBJ) zpuload zpuio

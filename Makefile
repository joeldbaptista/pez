include config.mk

SRC = util.c
OBJ = $(SRC:.c=.o)

all: pez pezas

.c.o:
	$(CC) -c $(CFLAGS) $<

pez: vm.o $(OBJ)
	$(CC) -o $@ vm.o $(OBJ) $(LDFLAGS)

pezas: as.o $(OBJ)
	$(CC) -o $@ as.o $(OBJ) $(LDFLAGS)

vm.o: vm.c pez.h util.h arg.h
as.o: as.c pez.h util.h arg.h
util.o: util.c util.h

clean:
	rm -f pez pezas *.o

.PHONY: all clean

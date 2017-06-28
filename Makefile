CC=gcc
CFLAGS= -g
TARGET=main
ODIR=obj
LIBS=-lz

DEPS = homm3_lod_file.h homm3_res_parser.h

OBJ = main.o homm3_lod_file.o homm3_res_parser.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o $(TARGET)


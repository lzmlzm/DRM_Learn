FLAGS=`pkg-config --cflags --libs libdrm`
FLAGS+=-Wall -O0 -g
FLAGS+=-D_FILE_OFFSET_BITS=64

all:
	aarch64-linux-gnu-gcc -o single-buffer single-buffer.c $(FLAGS)
	aarch64-linux-gnu-gcc -o double-buffer double-buffer.c $(FLAGS)
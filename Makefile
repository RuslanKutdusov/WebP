CC = g++
CFLAGS = -O3 -ffast-math -m64 -flto -march=native -funroll-loops -Wall -DLINUX
LDFLAGS = -lpng

all: transform.o utils.o lz77.o huffman_coding.o webp.o
	$(CC) -o webp_ transform.o utils.o lz77.o huffman_coding.o webp.o -lpng

transform.o: webp/vp8l/transform.cpp
	$(CC) $(CFLAGS) -c webp/vp8l/transform.cpp
	
utils.o: webp/utils/utils.cpp
	$(CC) $(CFLAGS) -c webp/utils/utils.cpp
	
lz77.o: webp/lz77/lz77.cpp
	$(CC) $(CFLAGS) -c webp/lz77/lz77.cpp
	
huffman_coding.o: webp/huffman_coding/huffman_coding.cpp
	$(CC) $(CFLAGS) -c webp/huffman_coding/huffman_coding.cpp
	
webp.o: webp.cpp
	$(CC) $(CFLAGS) -c webp.cpp
	
clean:
	rm transform.o
	rm utils.o
	rm lz77.o
	rm huffman_coding.o
	rm webp.o
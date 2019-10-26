CFLAGS ?= -std=c11 -O1 -g -Wall -pedantic

image.ppm: raytracer
	./raytracer > image.ppm

raytracer: raytracer.o vector.o
	$(CC) $(LDFLAGS) -o $@ $^ -lm

raytracer.o: raytracer.c vector.h
vector.o: vector.c vector.h

.SUFFIXES:
.SUFFIXES: .c .o

.PHONY: deps
deps:
	$(CC) -MM *.c

.PHONY: clean
clean:
	$(RM) image.ppm
	$(RM) raytracer
	$(RM) raytracer.o
	$(RM) vector.o

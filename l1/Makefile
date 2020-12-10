all: build

clear:
	rm main file_* -f

build: main.c clear
	gcc -lrt -pthread -g -o main main.c

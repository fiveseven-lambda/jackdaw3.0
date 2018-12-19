#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include "errorlist.h"
#include "load.h"

int samplerate = 44100;
short bitdepth = 16, channel = 2;
double length = 3.0;

_Bool successful = 1;

int main(int argc, char *argv[]){
	char *out_filename = "a.wav";
	int verbosity = 0;

	while(1){
		static struct option long_options[] = {
			{"version", no_argument, NULL, 'v'},
			{"help", no_argument, NULL, 'h'},
			{"out", required_argument, NULL, 'o'},
			{"verbose", no_argument, NULL, 1},
			{}
		};
		int opt = getopt_long(argc, argv, "vho:", long_options, NULL);
		if(opt == -1) break;
		else switch(opt){
			case 'v':
				puts("this is going to be jackdaw ver3.0");
				return 0;
			case 'o':
				out_filename = optarg;
				break;
			case 1:
				verbosity = 1;
				break;
			case 'h':
			default:
				printf("usage: %s [-v --version] [--verbose] [-o --out <file>] [-h --help]\n", argv[0]);
				return 0;
		}
	}

	for(; optind < argc; ++optind){
		FILE *fp = fopen(argv[optind], "r");
		if(fp == NULL) error(FOPEN_FAILED_ARG, argv[optind]);
		else{
			while(load(fp));
			if(fclose(fp) == EOF) error(FCLOSE_FAILED_ARG, argv[optind]);
		}
	}

	if(!successful) exit(EXIT_FAILURE);

	int out = open(out_filename, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if(out == -1) error(OPEN_FAILED, out_filename);

	int filesize = bitdepth / 8 * channel * samplerate * length + 44;

	if(ftruncate(out, filesize) == -1) error(FTRUNCATE_FAILED, out_filename, filesize);
	
	void *map = mmap(NULL, filesize, PROT_WRITE, MAP_SHARED, out, 0);
	if(map == MAP_FAILED) error(MMAP_FAILED, filesize, out_filename);

	if(verbosity >= 1) printf("samplerate = %d\nbitdepth = %d\nchannel = %d\nlength = %f\n", samplerate, bitdepth, channel, length);

	memcpy((char *)map, "RIFF", 4);
	((unsigned int *)map)[1] = filesize - 8;
	memcpy((char *)map + 8, "WAVEfmt ", 8);
	((unsigned int *)map)[4] = 16;
	((unsigned short *)map)[10] = 1;
	((unsigned short *)map)[11] = channel;
	((unsigned int *)map)[6] = samplerate;
	((unsigned int *)map)[7] = samplerate * bitdepth / 8 * channel;
	((unsigned short *)map)[16] = channel * bitdepth / 8;
	((unsigned short *)map)[17] = bitdepth;
	memcpy((char *)map + 36, "data", 4);
	((unsigned int *)map)[10] = filesize - 44;

	map += 44;
	for(int i = 0; i < samplerate * length; ++i){
		for(int j = 0; j < channel; ++j){
			int tmp = sin(2 * M_PI * 440 * i / samplerate) * (1 << bitdepth - 1);
			if(bitdepth == 8) ((unsigned char *)map)[i * channel + j] = tmp - (1 << 7);
			else if(bitdepth == 16) ((short *)map)[i * channel + j] = tmp;
			else if(bitdepth == 32) ((int *)map)[i * channel + j] = tmp;
			else error(INVALID_BITDEPTH, bitdepth);
		}
	}
	map -= 44;

	if(close(out) == -1) error(CLOSE_FAILED, out_filename);
	if(munmap(map, filesize) == -1) error(MUNMAP_FAILED, out_filename, filesize);
	return 0;
}

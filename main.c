#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

int samplerate = 44100;
short bitdepth = 16, channel = 2;
double length = 3.0;

_Bool load(FILE *);

enum ERROR{
	OPEN_FAILED, CLOSE_FAILED, FTRUNCATE_FAILED, MMAP_FAILED, MUNMAP_FAILED, FOPEN_FAILED_ARG, FCLOSE_FAILED_ARG, FOPEN_FAILED_IMPORT, FCLOSE_FAILED_IMPORT,
	INVALID_TOKEN = 100, UNTERMINATED_COMMENT, UNEXPECTED_END_OF_FILE, INVALID_TOKEN_AFTER_COMMAND, TOO_LONG_COMMAND, UNKNOWN_COMMAND, SYNTAX_SUBSTITUTION, TOO_LONG_ARGUMENT, TOO_LONG_MACRO, UNDEFINED_MACRO,
	INVALID_BITDEPTH = 200,
};

int error(enum ERROR err, ...){
	va_list ap;
	va_start(ap, err);
	fprintf(stderr, "error %d: ", err);
	switch(err){
		case OPEN_FAILED:
			fprintf(stderr, "failed to create output file \"%s\" (%s)\n", va_arg(ap, char *), strerror(errno));
			exit(EXIT_FAILURE);
		case CLOSE_FAILED:
			fprintf(stderr, "failed to close output file \"%s\" (%s)\n", va_arg(ap, char *), strerror(errno));
			exit(EXIT_FAILURE);
		case FTRUNCATE_FAILED:
			fprintf(stderr, "failed to truncate output file \"%s\" to size \"%d\" (%s)\n", va_arg(ap, char *), va_arg(ap, int), strerror(errno));
			exit(EXIT_FAILURE);
		case MMAP_FAILED:
			fprintf(stderr, "failed to map %d bytes to output file \"%s\" (%s)\n", va_arg(ap, int), va_arg(ap, char *), strerror(errno));
			exit(EXIT_FAILURE);
		case MUNMAP_FAILED:
			fprintf(stderr, "failed to unmap %d bytes of output file \"%s\" (%s)\n", va_arg(ap, int), va_arg(ap, char *), strerror(errno));
			exit(EXIT_FAILURE);
		case FOPEN_FAILED_ARG:
			fprintf(stderr, "failed to open input file \"%s\" (%s)\n", va_arg(ap, char *), strerror(errno));
			exit(EXIT_FAILURE);
		case FCLOSE_FAILED_ARG:
			fprintf(stderr, "failed to close input file \"%s\" (%s)\n", va_arg(ap, char *), strerror(errno));
			exit(EXIT_FAILURE);
		case FOPEN_FAILED_IMPORT:
			fprintf(stderr, "failed to open header file \"%s\" (%s)\n", va_arg(ap, char *), strerror(errno));
			exit(EXIT_FAILURE);
		case FCLOSE_FAILED_IMPORT:
			fprintf(stderr, "failed to close header file \"%s\" (%s)\n", va_arg(ap, char *), strerror(errno));
			exit(EXIT_FAILURE);
		case INVALID_TOKEN:
			fprintf(stderr, "invalid token \"%c\"\n", va_arg(ap, int));
			return 1;
		case UNTERMINATED_COMMENT:
			fprintf(stderr, "unterminated comment\n");
			exit(EXIT_FAILURE);
		case UNEXPECTED_END_OF_FILE:
			fprintf(stderr, "unexpected end of file\n");
			return 0;
		case INVALID_TOKEN_AFTER_COMMAND:
			fprintf(stderr, "invalid token \"%c\"\n", va_arg(ap, int));
			return 1;
		case TOO_LONG_COMMAND:
			fprintf(stderr, "too long command \"%s\"\n", va_arg(ap, char *));
			return 0;
		case UNKNOWN_COMMAND:
			fprintf(stderr, "unknown command \"%s\"\n", va_arg(ap, char *));
			return 1;
		case SYNTAX_SUBSTITUTION:
			fprintf(stderr, "syntax error of substitution\n");
			return 1;
		case TOO_LONG_ARGUMENT:
			fprintf(stderr, "too long argument \"%s\"\n", va_arg(ap, char *));
			return 0;
		case TOO_LONG_MACRO:
			fprintf(stderr, "too long macro \"%s\"\n", va_arg(ap, char *));
			return 2;
		case UNDEFINED_MACRO:
			fprintf(stderr, "undefined macro \"%s\"\n", va_arg(ap, char *));
			return 2;
		case INVALID_BITDEPTH:
			fprintf(stderr, "invalid bitdepth %d\n", va_arg(ap, int));
			return 2;
		default:
			return -1;
	}
}

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

	int out = open(out_filename, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if(out == -1) error(OPEN_FAILED, out_filename);

	int filesize = bitdepth / 8 * channel * samplerate * length + 44;

	if(ftruncate(out, filesize) == -1) error(FTRUNCATE_FAILED, out_filename, filesize);
	
	void *map = mmap(NULL, filesize, PROT_WRITE, MAP_SHARED, out, 0);
	if(map == MAP_FAILED) error(MMAP_FAILED, filesize, out_filename);

	for(; optind < argc; ++optind){
		FILE *fp = fopen(argv[optind], "r");
		if(fp == NULL) error(FOPEN_FAILED_ARG, argv[optind]);
		else{
			while(load(fp));
			if(fclose(fp) == EOF) error(FCLOSE_FAILED_ARG, argv[optind]);
		}
	}

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
			if(bitdepth == 8) ((char *)map)[i * channel + j] = tmp;
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

int get(FILE *fp){
	int ret = getc(fp);
	if(ret == '#'){
		ret = (getc(fp) == '#' ? '\n' : '#');
		for(int tmp = 0; tmp != ret; tmp = getc(fp)) if(tmp == EOF) error(UNTERMINATED_COMMENT);
		return getc(fp);
	}else return ret;
}

_Bool load(FILE *fp){
	static char command[10], arg[100];
	static struct{ char name[10], text[10]; } macro[10] = {};

	while(isspace(command[0] = get(fp)));
	if(command[0] == EOF){
		fclose(fp);
		return 0;
	}else if(isalpha(command[0])){
		int i = 1;
		_Bool execute = 1;
		while(isalpha(command[i] = get(fp))){
			if(execute){
				if(i == sizeof command - 1){
					command[i] = '\0';
					execute = error(TOO_LONG_COMMAND, command);
				}else ++i;
			}
		}
		char token;
		if(command[i] == '\n'){
			token = '\n';
			arg[0] = '\0';
		}else{
			_Bool not_too_long_argument = 1;
			if(command[i] == '{') token = '}';
			else if(isspace(command[i])) token = '\n';
			else return error(INVALID_TOKEN_AFTER_COMMAND, command[i]);
			for(int j = 0; (arg[j] = get(fp)) != token || (arg[j] = '\0');){
				if(j == sizeof arg - 1 && not_too_long_argument){
					arg[j] = 0;
					not_too_long_argument = error(TOO_LONG_ARGUMENT, arg);
				}
				if(arg[j] == EOF) return error(UNEXPECTED_END_OF_FILE);
				else if(arg[j] == '{'){
					char tmp[sizeof macro[0].name];
					for(int k = 0; (tmp[k] = get(fp)) != '}' || (tmp[k] = '\0'); ++k){
						if(k == sizeof tmp - 1){
							tmp[k] = '\0';
							error(TOO_LONG_MACRO, tmp);
							while(get(fp) != '}');
							break;
						}
					}
					for(int k = 0;; ++k){
						if(!macro[k].name[0]){
							error(UNDEFINED_MACRO, tmp);
							break;
						}else if(!strcmp(tmp, macro[k].name)){
							for(int l = 0; (arg[j + l] = macro[k].text[l]) || (j += l) && 0; ++l);
							break;
						}
					}
				}else if(not_too_long_argument) ++j;
			}
			execute = execute && not_too_long_argument;
		}
		command[i] = '\0';
		if(!strcmp(command, "comment")){
		}else if(!strcmp(command, "message")){
			fputs(arg, stdout);
			if(token == '\n') putchar('\n');
		}else if(!strcmp(command, "system")){
			system(arg);
		}else if(!strcmp(command, "import")){
			FILE *header = fopen(arg, "r");
			if(header == NULL) error(FOPEN_FAILED_IMPORT, arg);
			else while(load(header));
			if(fclose(header) == EOF) error(FCLOSE_FAILED_IMPORT, arg);
		}else if(!strcmp(command, "end")){
			return 0;
		}
		return 1;
	}else return error(INVALID_TOKEN, command[0]);
}

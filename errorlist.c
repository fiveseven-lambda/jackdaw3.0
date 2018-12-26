#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include "errorlist.h"

extern _Bool successful;

int error(enum ERROR err, ...){
	va_list ap;
	va_start(ap, err);
	fprintf(stderr, "error %d: ", err);
	successful = 0;
	switch(err){
		case OPEN_FAILED:
			fprintf(stderr, "failed to create output file \"%s\" (%s)\n", va_arg(ap, char *), strerror(errno));
			exit(EXIT_FAILURE);
		case CLOSE_FAILED:
			fprintf(stderr, "failed to close output file \"%s\" (%s)\n", va_arg(ap, char *), strerror(errno));
			exit(EXIT_FAILURE);
		case FTRUNCATE_FAILED:
			{
				char *a = va_arg(ap, char *);
				int b = va_arg(ap, int);
				fprintf(stderr, "failed to truncate output file \"%s\" to size \"%d\" (%s)\n", a, b, strerror(errno));
				exit(EXIT_FAILURE);
			}
		case MMAP_FAILED:
			{
				int a = va_arg(ap, int);
				char *b = va_arg(ap, char *);
				fprintf(stderr, "failed to map %d bytes to output file \"%s\" (%s)\n", a, b, strerror(errno));
				exit(EXIT_FAILURE);
			}
		case MUNMAP_FAILED:
			{
				int a = va_arg(ap, int);
				char *b = va_arg(ap, char *);
				fprintf(stderr, "failed to unmap %d bytes of output file \"%s\" (%s)\n", a, b, strerror(errno));
				exit(EXIT_FAILURE);
			}
		case NO_INPUT_FILE:
			fprintf(stderr, "no input file\n");
			exit(EXIT_FAILURE);
		case FOPEN_FAILED_ARG:
			fprintf(stderr, "failed to open input file \"%s\" (%s)\n", va_arg(ap, char *), strerror(errno));
			return 2;
		case FCLOSE_FAILED_ARG:
			fprintf(stderr, "failed to close input file \"%s\" (%s)\n", va_arg(ap, char *), strerror(errno));
			return 2;
		case FOPEN_FAILED_IMPORT:
			fprintf(stderr, "failed to open header file \"%s\" (%s)\n", va_arg(ap, char *), strerror(errno));
			return 1;
		case FCLOSE_FAILED_IMPORT:
			fprintf(stderr, "failed to close header file \"%s\" (%s)\n", va_arg(ap, char *), strerror(errno));
			return 1;
		case INVALID_TOKEN:
		case INVALID_TOKEN_AFTER_COMMAND:
			fprintf(stderr, "invalid token \"%c\"\n", va_arg(ap, int));
			return 1;
		case UNTERMINATED_COMMENT:
			fprintf(stderr, "unterminated comment\n");
			exit(EXIT_FAILURE);
		case UNEXPECTED_END_OF_FILE:
		case UNEXPECTED_END_OF_FILE_MACRO:
			fprintf(stderr, "unexpected end of file\n");
			return 0;
		case TOO_LONG_COMMAND:
			fprintf(stderr, "too long command \"%s...\"\n", va_arg(ap, char *));
			return 0;
		case UNKNOWN_COMMAND_LOWER:
		case UNKNOWN_COMMAND_UPPER:
			fprintf(stderr, "unknown command \"%s\"\n", va_arg(ap, char *));
			return 1;
		case SYNTAX_ERROR_SUBSTITUTION_NO_EQUAL:
			fprintf(stderr, "command begins with an upper letter, but no equal found\n");
			return 1;
		case SYNTAX_ERROR_SUBSTITUTION_NO_NAME:
			fprintf(stderr, "no name before equal\n");
			return 1;
		case SYNTAX_ERROR_SUBSTITUTION_EXTRA_CHARACTER_AFTER_NAME:
			fprintf(stderr, "extra character \'%c\' after name\n", va_arg(ap, int));
			return 1;
		case SYNTAX_ERROR_SUBSTITUTION_INVALID_NAME:
			fprintf(stderr, "name cannot include \'%c\'\n", va_arg(ap, int));
			return 1;
		case UNKNOWN_VARIABLE:
			fprintf(stderr, "unknown variable \"%s\"\n", va_arg(ap, char *));
			return 1;
		case SSCANF_FAILED_SAMPLERATE:
			{
				char *a = va_arg(ap, char *);
				int b = va_arg(ap, int);
				fprintf(stderr, "failed to set samplerate to \"%s\". samplerate = %d\n", a, b);
				return 1;
			}
		case SSCANF_FAILED_BITDEPTH:
			{
				char *a = va_arg(ap, char *);
				int b = va_arg(ap, int);
				fprintf(stderr, "failed to set bitdepth to \"%s\". bitdepth = %d\n", a, b);
				return 1;
			}
		case SSCANF_FAILED_CHANNEL:
			{
				char *a = va_arg(ap, char *);
				int b = va_arg(ap, int);
				fprintf(stderr, "failed to set channel to \"%s\". channel = %d\n", a, b);
				return 1;
			}
		case TOO_MANY_MACROS:
			fprintf(stderr, "too many macros defined\n");
			return 1;
		case DUPLICATED_MACROS:
			fprintf(stderr, "macro \"%s\" already defined\n", va_arg(ap, char *));
			return 1;
		case TOO_LONG_ARGUMENT:
			fprintf(stderr, "too long argument \"%s...\"\n", va_arg(ap, char *));
			return 0;
		case TOO_LONG_MACRO:
			fprintf(stderr, "too long macro \"%s...\"\n", va_arg(ap, char *));
			return 2;
		case UNDEFINED_MACRO:
		case UNDEFINED_MACRO_UNDEFINE:
			fprintf(stderr, "undefined macro \"%s\"\n", va_arg(ap, char *));
			return 2;
		case INVALID_BITDEPTH:
			fprintf(stderr, "invalid bitdepth %d\n", va_arg(ap, int));
			return 2;
		default:
			return -1;
	}
}

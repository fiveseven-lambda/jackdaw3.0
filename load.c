#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include "errorlist.h"
#include "load.h"

extern int samplerate;
extern short bitdepth, channel;
extern double length;

extern _Bool successful;

static int get(FILE *fp){
	int ret = getc(fp);
	if(ret == '#'){
		ret = (getc(fp) == '#' ? '\n' : '#');
		for(int tmp = 0; tmp != ret; tmp = getc(fp)) if(tmp == EOF) error(UNTERMINATED_COMMENT);
		return getc(fp);
	}else return ret;
}

typedef struct{
	enum{ Absolute, Relative } flag;
	union{ unsigned uvalue[2]; double dvalue; } frequency;
	double pan[2];
	double envelope[100][2];
	double phase;
} Instrument[100];
struct{
	Instrument *instrument;
	double velocity;
	double frequency;
	double start, end;
} score_instrument[100];
struct{
	char name[100];
	Instrument *instrument;
} instrument[100];

_Bool load(FILE *fp){
	static char command[20], arg[10000];
	static struct{ char name[sizeof command], text[sizeof arg]; } macro[100] = {};

	while(isspace(command[0] = get(fp)));
	if(command[0] == EOF){
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
						}else if(tmp[k] == EOF){
							return error(UNEXPECTED_END_OF_FILE_MACRO);
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
		if(execute){
			if(islower(command[0])){
				if(!strcmp(command, "comment")){
					return 1;
				}else if(!strcmp(command, "message")){
					fputs(arg, stdout);
					if(token == '\n') putchar('\n');
					return 1;
				}else if(!strcmp(command, "system")){
					system(arg);
					return 1;
				}else if(!strcmp(command, "import")){
					FILE *header = fopen(arg, "r");
					if(header == NULL) return error(FOPEN_FAILED_IMPORT, arg);
					while(load(header));
					if(fclose(header) == EOF) return error(FCLOSE_FAILED_IMPORT, arg);
					return 1;
				}else if(!strcmp(command, "end")){
					return 0;
				}else if(!strcmp(command, "abort")){
					successful = 0;
					return 0;
				}else if(!strcmp(command, "undefine")){
					for(int j = 0;; ++j){
						if(j == sizeof macro / sizeof macro[0] || !macro[j].name[0]) return error(UNDEFINED_MACRO_UNDEFINE, arg);
						else if(!strcmp(arg, macro[j].name)){
							int k;
							for(k = 0; j < sizeof macro / sizeof macro[0] && macro[k].name[0]; ++k);
							strcpy(macro[j].name, macro[k - 1].name);
							strcpy(macro[j].text, macro[k - 1].text);
							macro[k - 1].name[0] = '\0';
							return 1;
						}
					}
				}else if(!strcmp(command, "melody")){
					return 1;
				}else if(!strcmp(command, "percussion")){
					return 1;
				}else return error(UNKNOWN_COMMAND_LOWER, command);
			}else{
				char *name, *val;
				for(val = arg; *val != '='; ++val) if(*val == '\0') return error(SYNTAX_ERROR_SUBSTITUTION_NO_EQUAL);
				for(name = arg; isspace(*name); ++name) if(name == val - 1) return error(SYNTAX_ERROR_SUBSTITUTION_NO_NAME);
				for(char *j = name; j < val; ++j){
					if(isspace(*j)){
						*j = '\0';
						for(++j; j < val; ++j) if(!isspace(*j)) return error(SYNTAX_ERROR_SUBSTITUTION_EXTRA_CHARACTER_AFTER_NAME, *j);
					}else if(!(isalnum(*j) || *j == '_')) return error(SYNTAX_ERROR_SUBSTITUTION_INVALID_NAME, *j);
				}
				*val++ = '\0';
				if(!strcmp(command, "Set")){
					if(!strcmp(name, "samplerate")){ if(sscanf(val, "%d", &samplerate) != 1) error(SSCANF_FAILED_SAMPLERATE, val, samplerate); }
					else if(!strcmp(name, "bitdepth")){ if(sscanf(val, "%hd", &bitdepth) != 1) error(SSCANF_FAILED_BITDEPTH, val, bitdepth); }
					else if(!strcmp(name, "channel")){ if(sscanf(val, "%hd", &channel) != 1) error(SSCANF_FAILED_CHANNEL, val, channel); }
					else error(UNKNOWN_VARIABLE, name);
					return 1;
				}else if(!strcmp(command, "Define")){
					for(int j = 0;; ++j){
						if(j == sizeof macro / sizeof macro[0]) return error(TOO_MANY_MACROS);
						else if(!macro[j].name[0]){
							strcpy(macro[j].name, name);
							strcpy(macro[j].text, val);
							return 1;
						}else if(!strcmp(name, macro[j].name)) return error(DUPLICATED_MACROS, name);
					}
				}else if(!strcmp(command, "Instrument")){
					return 1;
				}else return error(UNKNOWN_COMMAND_UPPER, command);
			}
		}else return 1;
	}else return error(INVALID_TOKEN, command[0]);
}

void write8bit(unsigned char *map){
	for(int i = 0; i < samplerate * length; ++i){
		for(int j = 0; j < channel; ++j){
			map[i * channel + j] = sin(2 * M_PI * 440 * i / samplerate) * (1 << 7) - (1 << 7) - 1;
		}
	}
}

void write16bit(short *map){
	for(int i = 0; i < samplerate * length; ++i){
		for(int j = 0; j < channel; ++j){
			map[i * channel + j] = sin(2 * M_PI * 44e+9 * exp((i + samplerate * 6) / 200000.) / samplerate / samplerate) * (1 << 15) - 1;
		}
	}
}

void write32bit(int *map){
	for(int i = 0; i < samplerate * length; ++i){
		for(int j = 0; j < channel; ++j){
			map[i * channel + j] = sin(2 * M_PI * 440 * i / samplerate) * (1 << 31);
		}
	}
}

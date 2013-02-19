
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zpu.h>

void print_usage();
char* readline();

int main(int argc, char* argv[])
{
	int fd;
	int p;
	
	if (argc > 1 && (strcmp("--help", argv[1]) == 0 || strcmp("-h", argv[1]) == 0))
	{
		print_usage(argc, argv);
		return 0;
	}
	
	ZPU_OPEN_RW(fd);
	if (fd < 0)
	{
		printf("Device file /dev/zpu could not be opened.\n");
		printf("Is modzpu loaded?\n");
		exit(1);
	}
	
	p = fork();

	if (p == 0)
	{
		char buf[1025];
		int  r;
		while (1 == 1)
		{
			r = read(fd, &(buf), 1024);
			buf[r] = 0x00;
			
			printf("\n\033[01;31moutput (%i) > \033[0m%s\n", r, buf);
		}
	}
	else
	{
		while(1 == 1)
		{
			printf("\033[01;32minput > \033[0m");
			char* line = readline();
	
			write(fd, line, strlen(line));
	
			free(line);
		}
	}
	
	return 0;
}

char* readline()
{
	int   len  = 128;
	int   lenm = len;
	int   c;
	
	char* l   = malloc(len);
	char* lp  = l;
	
	for(;;)
	{
		c = fgetc(stdin);
		
		if (c == EOF)
		{
			break;
		}
		
		if (-- len == 0)
		{
			len = lenm;
			char* ln = realloc(lp, lenm *= 2);
			
			l  = ln + (l - lp);
			lp = ln;
		}
		
		if (c == '\n')
		{
			break;
		}
			
		*l++ = c;
	}
	
	*l = 0x00;
	return lp;
}

void print_usage(int argc, char* argv[])
{
	printf("ZPUIO 1.0\n\n");
	printf("Oliver Erxleben <oliver.erxleben@hs-osnabrueck.de>\n");
	printf("Martin Helmich <martin.helmich@hs-osnabrueck.de>\n\n");
	printf("University of Applied Sciences Osnabrück\n\n");
	printf("USAGE:\n    %s\n\n", argv[0]);
	printf("DESCRIPTION:\n");
	printf("    Input and output interface for the ZPU processor. This programm\n");
	printf("    implements a small terminal, accepting user input and displaying\n");
	printf("    ZPU output to screen.\n");
}

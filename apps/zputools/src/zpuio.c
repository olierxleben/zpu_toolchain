
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
	
	ZPU_OPEN_RW(fd);
	
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

void print_usage()
{
	printf("Hilfetext hier einfügen...\n");
}

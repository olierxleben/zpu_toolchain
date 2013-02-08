
#include <stdlib.h>
#include <stdio.h>

#include "zpu.h"

void print_usage();

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		print_usage();
		exit(1);
	}
	
	char* filename = argv[1];
	
	printf("Lade Programm aus %s...\n", filename);
	
	if(zpu_load_from_file(filename) != 0)
	{
		printf("%s\n", zpu_error());
		return 1;
	}
	
	printf("Programm geladen.\n");
	
	return 0;
}

void print_usage()
{
	printf("Hilfetext hier einfügen...\n");
}


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "zpu.h"

void print_usage();

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		printf("ERROR: No argument given!\n\n");
	
		print_usage(argc, argv);
		return 1;
	}
	
	if (argc > 1 && (strcmp("--help", argv[1]) == 0 || strcmp("-h", argv[1]) == 0))
	{
		print_usage(argc, argv);
		return 0;
	}
	
	char* filename = argv[1];
	
	printf("Lade Programm aus %s...\n", filename);
	
	if (zpu_load_from_file(filename) != 0)
	{
		printf("%s\n", zpu_error());
		return 1;
	}
	
	printf("Programm geladen.\n");
	
	return 0;
}

void print_usage(int argc, char* argv[])
{
	printf("ZPULOAD 1.0\n\n");
	printf("Oliver Erxleben <oliver.erxleben@hs-osnabrueck.de>\n");
	printf("Martin Helmich <martin.helmich@hs-osnabrueck.de>\n\n");
	printf("University of Applied Sciences Osnabrück\n\n");
	printf("USAGE:\n    %s <hexfile>\n\n", argv[0]);
	printf("DESCRIPTION:\n");
	printf("    Loads a program defined by <hexfile> into the RAM of an ZPU board.\n");
	printf("    The input file must be an Intel HEX formatted binary file.\n");
	printf("    Requires the zpu kernel module to be loaded.\n");
}

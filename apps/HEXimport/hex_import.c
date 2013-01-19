#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "structs/intel_hex.h"
#include "structs/importer.h"

int main (void) {
	
	imp__load_file();
	imp__display();
	hex__display();
	
	
	return 0;
}
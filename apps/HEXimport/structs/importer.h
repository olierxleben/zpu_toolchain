int imp__load_file();
void imp__display();

struct imp_struct{
	unsigned char read_buf[16]; // the read buffer
	FILE *file;									// pointer to file
	
} importer;

int imp__load_file() {
	printf("laoding...\n\n");
	
	FILE *fil;
  int line_length = 80;
  char line[line_length];
	
	fil = fopen("testhex.hex", "r");

  if (fil == NULL) {
      printf("\nFile cannot be opened!\n");
      return(-1);
  }
	
	int i = 0;
  while( fgets (line, line_length, fil) ) {
    printf("%.*s",line_length, line);
		
		strcat(hex.data,line); // concate lines
  }

  if( !feof(fil) )
    printf("Error during read!\n" );

	importer.file = fil;
	
  fclose (fil);
	
	return 0;
}

void imp__display() {
	printf("\nData in Importer:\n");
	printf("Importer File Address:%p\n", importer.file);
}

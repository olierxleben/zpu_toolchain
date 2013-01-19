struct IntelHex {
	int byte_count;
	int address;
	int type;
	char data[1024];
	int checksum; 
} hex;

void hex__init(){
	//TODO Implementation
};
	
void hex__display() {
	printf("\nData in Hex:\n");
	printf("hex data:\n%s\n", hex.data);
}
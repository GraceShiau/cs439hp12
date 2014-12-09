#include "libc.h"

int main() {

	const long fileHandle = open(".");
	if (fileHandle == -1006) {
		puts("ls: .: access denied\n");
		return 0;	
	}
	else if(fileHandle < 0)	{
		puts("ls failed\n");
		return 0;
	}


	const long fileLength = getlen(fileHandle);
	for(int a = 0; a < fileLength; a += 16)
	{
		char fileName[16];
		read(fileHandle, fileName, 16);
		fileName[12] = 0;
		if (fileName[0] != '.') {
			puts(fileName);
			puts("\n");		
		}
	}
    return 0;
}

#include "libc.h"

int main(int argc, char** argv) {

	if(argc < 2)
	{
		return 0;
	}

	for(int a = 1; a < argc; ++a)
	{
		const long fileHandle = open(argv[a]);
		if (fileHandle == -1006) {
			puts("cat: ");
			puts(argv[a]);
			puts(": access denied\n");
			return 0;	
		}
		else if(fileHandle < 0) {
			puts("cat: ");
			puts(argv[a]);
			puts(": No such file or directory\n");
			return 0;
		}

		const long fileLength = getlen(fileHandle);
		int filePosition = 0;
		while(filePosition < fileLength)
		{
			char fileContents[513];
			const long bytesRead = read(fileHandle, fileContents, fileLength - filePosition);
			if(bytesRead > 0)
			{
				filePosition+= bytesRead;
			}
			else
			{
				break;
			}

			fileContents[bytesRead] = 0;
			puts(fileContents);
		}
	}

    return 0;
}

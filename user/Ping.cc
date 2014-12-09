extern "C" {
#include "libc.h"
#include "sys.h"
}


int main(int argc, char** args)
{
	const char* addrStr = args[1];

	int i = 0;
	unsigned char addr[4];
	int j = 0;
	unsigned char byte = 0;
	while (addrStr[i] != 0)
	{
		if (addrStr[i] == '.')
		{
			addr[j] = byte;
			j++;
			byte = 0;
		}
		else
		{
			byte = byte * 10 + ((unsigned char)addrStr[i]-48);
		}

		++i;
	}
	addr[3] = byte;


	for(int a = 0; a < 60; ++a)
	{
		puts("Pinging: ");
		putdec((unsigned int)addr[0]);
		puts(".");
		putdec((unsigned int)addr[1]);
		puts(".");
		putdec((unsigned int)addr[2]);
		puts(".");
		putdec((unsigned int)addr[3]);
		puts("\n");
		Ping(addr);
		Sleep(200);
	}
}

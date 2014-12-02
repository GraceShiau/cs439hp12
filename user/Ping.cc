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
	int byte = 0;
	while (addrStr[i] != 0)
	{
		if (addrStr[i] == '.')
		{
			addr[j] = byte;
			j++;
			byte = 0;
		}

		byte = byte * 10 + (addrStr[i]-48);
	}
	addr[3] = byte;

	for(int a = 0; a < 15; ++a)
	{
		Ping(addr);
		Sleep(1000);
	}
}

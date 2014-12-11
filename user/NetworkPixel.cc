extern "C" {
#include "libc.h"
#include "sys.h"
}

#include "libcc.h"

int main(int argc, char** args)
{
	if(argc < 2)
	{
		puts("ERROR: Requires arguments <dest ip> optional:<dest port> optional:<listen port>.\n");
		return -1;
	}
	const char* addrStr = args[1];

	int i = 0;
	unsigned char addr[4];
	int j = 0;
	unsigned char byte = 0;
	int dotCount = 0;
	while (addrStr[i] != 0)
	{
		if (addrStr[i] == '.')
		{
			addr[j] = byte;
			j++;
			byte = 0;
			++dotCount;
		}
		else
		{
			byte = byte * 10 + ((unsigned char)addrStr[i]-48);
		}

		++i;
	}
	if(dotCount != 3)
	{
		puts("ERROR: Invalid ip address.\n");
		return -1;
	}
	addr[3] = byte;

	putdec((unsigned int)addr[0]);
	puts(".");
	putdec((unsigned int)addr[1]);
	puts(".");
	putdec((unsigned int)addr[2]);
	puts(".");
	putdec((unsigned int)addr[3]);
	puts("\n");
	int listenPort = 17;
	int sendPort = 17;
	if (argc >= 3)
	{
		sendPort = ((unsigned char)args[2][0]-48);
	}
	if (argc >= 4)
	{
		listenPort = ((unsigned char)args[3][0]-48);
	}
	puts("Listen port is: "); putdec(listenPort); puts(" send port is: "); putdec(sendPort); puts(".\n");
	const long screenBufferId = GetScreenBuffer(80, 60);


	unsigned char buf[80 * 60];
	int count = 0;
	int color = 2;
	for(int a = 0; a < 80 * 60; ++a)
	{
		buf[a] = 2;
	}

	const int socketDescriptor = OpenSocket(1, listenPort);
	if(socketDescriptor < 0)
	{
		puts("Failed to open socket.\n");
	}

	puts("Successfully opened socket descriptor on port 17.\n");
	int bufferX = 20;
	int bufferY = 20;
	while(1)
	{

		unsigned char buffer[512];
		unsigned char ip[4];
		const int bytesFromNetwork = ReadSocket(socketDescriptor, ip, buffer, 512);

		if (bytesFromNetwork > 0)
		{
			puts((const char*)buffer);
			puts("\n");
			switch(((const char*)buffer)[0])
			{
			case 'w':
				bufferY-=1;
				break;
			case 'a':
				bufferX-=1;
				break;
			case 's':
				bufferY+=1;
				break;
			case 'd':
				bufferX+=1;
				break;
			}

			buf[bufferX * 60 + bufferY] = 4;
		}




		++count;
		const int keyPressCount = GetQueuedKeyPressCount();

		if(keyPressCount > 0)
		{
			char* keyBuffer = new char[keyPressCount];
			GetQueuedKeyPresses(keyBuffer, keyPressCount);
			for(int a = 0; a < keyPressCount; ++a)
			{
				const char data[2] = {keyBuffer[a], 0};

				WriteSocket(socketDescriptor, addr, (unsigned char*)data, strlen(data) + 1, sendPort);
			}

			delete[] keyBuffer;
		}

		LockScreenBuffer(screenBufferId);
		if(WriteScreenBuffer(screenBufferId, buf) < 0)
		{
			puts("Failed to write Test Game screenbuffer.\n");
		}
		UnlockScreenBuffer(screenBufferId);

		Sleep(16);
	}

	return 0;
}

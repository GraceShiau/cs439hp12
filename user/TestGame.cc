extern "C" {
#include "libc.h"
#include "sys.h"
}

#include "libcc.h"
#include "smile.h"

int main()
{
	const long screenBufferId = GetScreenBuffer();

	char smileBuffer[512];
	const long smileId = open("smile.pic");
	if(smileId < 0)
	{
		puts("failed to load smile.\n");
		return -1;
	}

	unsigned char buf[80 * 60];
	int count = 0;
	int color = 2;
	for(int a = 0; a < 80 * 60; ++a)
	{
		buf[a] = 2;
	}
	const long bytesRead = read(smileId, (void*)smileBuffer, 512);
	char smile[256];

	smileBuffer[bytesRead] = 0;
	for(int y = 0; y < 16; ++y)
	{
		for(int x = 0; x < 16; ++x)
		{
			smile[x * 16 + y] = smileBuffer[y * 17 + x];
		}
	}

	Smile mySmile;
	mySmile.x = 15;
	mySmile.y = 18;

	const int socketDescriptor = OpenSocket(1, 16);
	if(socketDescriptor < 0)
	{
		puts("Failed to open socket.\n");
	}

	puts("Successfully opened socket descriptor on port 16.\n");
	unsigned char destIP[4] = {192, 168, 7, 4};
	const char* data = "abcdefghijklmnopqrstuvwxyz";

	WriteSocket(socketDescriptor, destIP, (unsigned char*)data, strlen(data) + 1);

	while(1)
	{

		unsigned char buffer[512];
		unsigned char ip[4];
		const int bytesFromNetwork = ReadSocket(socketDescriptor, ip, buffer, 512);

		if (bytesFromNetwork > 0)
		{
			putdec(bytesFromNetwork);
			puts("\n");
			puts((const char*)buffer);
			puts("\n");
		}


		for(int a = 0; a < 80 * 60; ++a)
		{
			buf[a] = 2;
		}

		++count;
		const int keyPressCount = GetQueuedKeyPressCount();

		if(keyPressCount > 0)
		{
			char* keyBuffer = new char[keyPressCount];
			GetQueuedKeyPresses(keyBuffer, keyPressCount);
			for(int a = 0; a < keyPressCount; ++a)
			{
				switch(keyBuffer[a])
				{
				case 'a':
					mySmile.x-= 1;
					break;
				case 'd':
					mySmile.x+= 1;
					break;
				case 'w':
					mySmile.y-= 1;
					break;
				case 's':
					mySmile.y+= 1;
					break;
				}
			}

			delete[] keyBuffer;
		}

		for(int y = 0; y < 16; ++y)
		{
			for(int x = 0; x < 16; ++x)
			{
				buf[(x + mySmile.x) * 60 + y + mySmile.y] = smile[x*16 + y] != '0' ? 4 : 0;
			}
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

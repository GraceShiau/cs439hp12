extern "C" {
#include "libc.h"
#include "sys.h"
}

#include "libcc.h"
#include "smile.h"

static const int leftPaddleX = 23;
static const int rightPaddleX = 127;

static const int paddleWidth = 10;
static const int paddleHeight = 50;

int main(int argc, char** args)
{
	if(argc != 2)
	{
		puts("ERROR: Requires argument <game server ip>.\n");
		return -1;
	}
	//get server ip address
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

	const int width = 160;
	const int height = 120;
	const long screenBufferId = GetScreenBuffer(width, height);

	char smileBuffer[512];
	const long smileId = open("smile.pic");
	if(smileId < 0)
	{
		puts("failed to load smile.\n");
		return -1;
	}

	char sadBuffer[2048];
	const long sadId = open("sad.pic");
	if(sadId < 0)
	{
		puts("failed to load sad.\n");
		return -1;
	}

	char happyBuffer[2048];
	const long happyId = open("happy.pic");
	if(happyId < 0)
	{
		puts("failed to load happy.\n");
		return -1;
	}

	unsigned char buf[width * height];
	int count = 0;
	int color = 2;
	for(int a = 0; a < width * height; ++a)
	{
		buf[a] = 2;
	}

	long bytesRead = read(smileId, (void*)smileBuffer, 512);
	char smile[256];

	smileBuffer[bytesRead] = 0;
	for(int y = 0; y < 16; ++y)
	{
		for(int x = 0; x < 16; ++x)
		{
			smile[x * 16 + y] = smileBuffer[y * 17 + x];
		}
	}

	bytesRead = readFully(sadId, (void*)sadBuffer, 2048);
	char sad[1600];

	sadBuffer[bytesRead] = 0;
	for(int y = 0; y < 40; ++y)
	{
		for(int x = 0; x < 40; ++x)
		{
			sad[x * 40 + y] = sadBuffer[y * 41 + x];
		}
	}

	bytesRead = readFully(happyId, (void*)happyBuffer, 2048);
	char happy[1600];

	happyBuffer[bytesRead] = 0;
	for(int y = 0; y < 40; ++y)
	{
		for(int x = 0; x < 40; ++x)
		{
			happy[x * 40 + y] = happyBuffer[y * 41 + x];
		}
	}

	Smile mySmile;
	mySmile.x = 15;
	mySmile.y = 18;

	Smile myFace;
	myFace.x = 20;
	myFace.y = 0;

	int leftPaddleY = 60;
	int rightPaddleY = 60;
	const int socketDescriptor = OpenSocket(1, 3);
	if(socketDescriptor < 0)
	{
		puts("Failed to open socket.\n");
	}

	bool isSad = false;
	bool isHappy = false;

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

			if(buffer[0] == 'b' && buffer[1] == ':')
			{
				puts("Received ball position update: ");
				int ballX;
				int ballY;
				memcpy(&ballX, buffer + 2, 4);
				memcpy(&ballY, buffer + 6, 4);
				memcpy(&leftPaddleY, buffer + 10, 4);
				memcpy(&rightPaddleY, buffer + 14, 4);
				putdec(ballX); puts(", "); putdec(ballY); puts("\n");
				mySmile.x = ballX;
				mySmile.y = ballY;
				isSad = false;
				isHappy = false;
			}
			else if(buffer[0] == 'g' && buffer[1] == ':')
			{
				puts("Received game result: ");
				if (buffer[2] == 'l')
				{
					isSad = true;
					puts("YOU LOST\n");
				}
				else
				{
					isHappy = true;
					puts("YOU WON\n");
				}
			}
		}


		for(int a = 0; a < width * height; ++a)
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
				/*case 'a':
					mySmile.x-= 1;
					break;
				case 'd':
					mySmile.x+= 1;
					break;
				case 'w':
					mySmile.y-= 1;
					break;*/
				case 's':
				{
					const char* data = "s:";
					WriteSocket(socketDescriptor, addr, (unsigned char*)data, strlen(data) + 1, 2);
				}
				break;
				case 'w':
				{
					const char* data = "w:";
					WriteSocket(socketDescriptor, addr, (unsigned char*)data, strlen(data) + 1, 2);
				}
				break;
				case 't':
				{
					const char* data = "c:";
					WriteSocket(socketDescriptor, addr, (unsigned char*)data, strlen(data) + 1, 2);

					//isSad = false;
					//isHappy = false;
				}
				break;
				}
			}

			delete[] keyBuffer;
		}

		if (isSad)
		{
			for (int y = 0; y < 120; ++y)
			{
				for (int x = 0; x < 120; ++x)
				{
					buf[(x + myFace.x) * height + y + myFace.y] = sad[x/3*40 + y/3] != '0' ? 1 : 8;
				}
			}
		}
		else if (isHappy)
		{
			for (int y = 0; y < 120; ++y)
			{
				for (int x = 0; x < 120; ++x)
				{
					buf[(x + myFace.x) * height + y + myFace.y] = happy[x/3*40 + y/3] != '0' ? 12 : 15;
				}
			}
		}
		else
		{
			for(int y = 0; y < 16; ++y)
			{
				for(int x = 0; x < 16; ++x)
				{
					buf[(x + mySmile.x) * height + y + mySmile.y] = smile[x*16 + y] != '0' ? 4 : 0;
				}
			}
		}

		

		for(int x = leftPaddleX; x < leftPaddleX + paddleWidth; ++x)
		{
			for(int y = leftPaddleY - paddleHeight / 2; y < leftPaddleY + paddleHeight / 2; ++y)
			{
				if(x < 0 || x >= width || y < 0 || y >= height)
					continue;
				buf[x * height + y] = 3;
			}
		}
		for(int x = rightPaddleX; x < rightPaddleX + paddleWidth; ++x)
		{
			for(int y = rightPaddleY - paddleHeight / 2; y < rightPaddleY + paddleHeight / 2; ++y)
			{
				if(x < 0 || x >= width || y < 0 || y >= height)
					continue;
				buf[x * height + y] = 3;
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

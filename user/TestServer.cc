extern "C" {
#include "libc.h"
#include "sys.h"
}

#include "libcc.h"

static const int width = 160;
static const int height = 120;
static const int leftPaddleEdge = 33;
static const int rightPaddleEdge = 127;

struct Vector
{
	int x;
	int y;
	Vector operator+(const Vector &vec)
	{
		return Vector(this->x+vec.x, this->y+vec.y);
	}

	Vector operator+=(const Vector &vec)
	{
		this->x += vec.x;
		this->y += vec.y;
		return *this;
	}

	Vector(int x, int y) : x(x), y(y)
	{}
};

class Ball
{
public:
	Vector position;
	Vector velocity;

public:
	void Move()
	{
		position += velocity;
		if (position.x < 0)
		{
			position.x = -position.x;
			velocity.x = -velocity.x;
		}
		if (position.y < 0)
		{
			position.y = -position.y;
			velocity.y = -velocity.y;
		}
		if ((position.x + 16) > width)
		{
			position.x = position.x - (position.x + 16 - width)*2;
			velocity.x = -velocity.x;
		}
		if ((position.y + 16) > height)
		{
			position.y = position.y - (position.y + 16 - height)*2;
			velocity.y = -velocity.y;
		}
	}

	Ball(Vector position, Vector velocity) : position(position), velocity(velocity)
	{}

};

class Game
{
public:
	Ball ball;

public:
	void Update()
	{
		ball.Move();
	}

	void Start()
	{
		Vector pos(72,52);
		Vector vel(5,5);
		ball = Ball(pos, vel);
	}

	Game() : ball(Vector(0,0), Vector(0,0))
	{

	}
};

struct Client
{
	unsigned char ip[4];
	int playerNum;
};

int main(int argc, char** args)
{
	int listenPort = 2;
	int sendPort = 3;
	puts("Listen port is: "); putdec(listenPort); puts(" send port is: "); putdec(sendPort); puts(".\n");
	

	const int socketDescriptor = OpenSocket(1, listenPort);
	if(socketDescriptor < 0)
	{
		puts("Failed to open socket.\n");
	}

	puts("Successfully opened socket descriptor on port 17.\n");

	Game game;

	Client* connections[256] = {nullptr};
	List<Client*> clients;

	int paddlePositions[2] = {60, 60};
	while(1)
	{

		unsigned char buffer[512];
		unsigned char ip[4];
		const int bytesFromNetwork = ReadSocket(socketDescriptor, ip, buffer, 512);

		if (bytesFromNetwork > 0)
		{
			puts((const char*)buffer);
			puts("\n");

			//check for connections;

			if(buffer[1] == ':')
			{
				switch(buffer[0])
				{
				case 'c':
					{
						if (connections[ip[3]] == nullptr)
						{
							Client* client = new Client();
							memcpy(client->ip, ip, 4);
							client->playerNum = clients.size;
							connections[ip[3]] = client;
							clients.Push(client);
							puts("Received a new connection request ");
							putdec(ip[3]);
							puts("\n");
							if(clients.size >= 2)
							{
								game.Start();
							}
						}
					}
					break;

				case 'w':
				{
					paddlePositions[connections[ip[3]]->playerNum] -= 2;
				}
				break;
				case 's':
				{
					paddlePositions[connections[ip[3]]->playerNum] += 2;
				}
				break;
				}
			}
			
		}

		if(clients.size >= 2)
		{
			game.Update();
		}

		List<Client*>::ListNode* node = clients.GetHead();
		while(node != nullptr)
		{
			unsigned char data[18];
			data[0] = 'b'; data[1] = ':';
			memcpy(data + 2, &game.ball.position.x, 4);
			memcpy(data + 6, &game.ball.position.y, 4);
			memcpy(data + 10, &paddlePositions[0], 4);
			memcpy(data + 14, &paddlePositions[1], 4);

	 		WriteSocket(socketDescriptor, node->value->ip, (unsigned char*)data, 18, sendPort);
			node = node->next;
		}
		const int keyPressCount = GetQueuedKeyPressCount();

		// if(keyPressCount > 0)
		// {
		// 	char* keyBuffer = new char[keyPressCount];
		// 	GetQueuedKeyPresses(keyBuffer, keyPressCount);
		// 	for(int a = 0; a < keyPressCount; ++a)
		// 	{
		// 		const char data[2] = {keyBuffer[a], 0};

		// 		WriteSocket(socketDescriptor, addr, (unsigned char*)data, strlen(data) + 1, sendPort);
		// 	}

		// 	delete[] keyBuffer;
		// }

		//unsigned char* buffer;
		//buffer[0] = 'b';
		//memcpy(buffer + 1, game)
		Sleep(24);
	}

	return 0;
}

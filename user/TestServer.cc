extern "C" {
#include "libc.h"
#include "sys.h"
}

#include "libcc.h"

static const int width = 160;
static const int height = 120;
static const int leftPaddleEdge = 33;
static const int rightPaddleEdge = 127;
static const int paddleHeight = 50;
static int paddlePositions[2] = {60, 60};

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
	int Move()
	{
		position += velocity;
		if (position.x < 17)
		{
			velocity.x = 0;
			velocity.y = 0;
			return 1;
		}
		if (position.y < 0)
		{
			position.y = -position.y;
			velocity.y = -velocity.y;
		}
		if ((position.x + 16) > (width - 17))
		{
			velocity.x = 0;
			velocity.y = 0;
			return 0;
		}
		if ((position.y + 16) > height)
		{
			position.y = position.y - (position.y + 16 - height)*2;
			velocity.y = -velocity.y;
		}

		if ((position.x < leftPaddleEdge) && ((position.x - velocity.x) >= leftPaddleEdge))
		{
			int paddle = paddlePositions[0];
			if ((position.y > (paddle - paddleHeight/2 - 16)) && (position.y < (paddle + paddleHeight/2)))
			{
				position.x = position.x - (position.x - leftPaddleEdge)*2;
				velocity.x = -velocity.x;
			}
			else
			{
				return -2;
			}
		}

		if (((position.x + 16) > rightPaddleEdge) && ((position.x + 16 - velocity.x) <= rightPaddleEdge))
		{
			int paddle = paddlePositions[1];
			if ((position.y > (paddle - paddleHeight/2 - 16)) && (position.y < (paddle + paddleHeight/2)))
			{
				position.x = position.x - (position.x + 16- rightPaddleEdge)*2;
				velocity.x = -velocity.x;
			}
			else
			{
				return -2;
			}
		}
		return -1;
	}

	Ball(Vector position, Vector velocity) : position(position), velocity(velocity)
	{}

};

class Game
{
public:
	Ball ball;
	int state;

public:
	void Update()
	{
		if (! isGameOver())
		{
			this->state = ball.Move();
		}
	}

	void Start()
	{
		Vector pos(72,52);
		Vector vel(3,3);
		ball = Ball(pos, vel);
		paddlePositions[0] = 60;
		paddlePositions[1] = 60;
	}

	void Connect()
	{
		Vector pos(72,52);
		Vector vel(0,0);
		ball = Ball(pos, vel);
		this->state = -1;
	}

	bool isGameOver()
	{
		return (this->state > -1);
	}

	bool isPaddleFrozen()
	{
		return (this->state == -2);
	}

	int getWinner()
	{
		return this->state;
	}

	Game() : ball(Vector(72,52), Vector(0,0)), state(-2)
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

	Game game;

	Client* connections[256] = {nullptr};
	List<Client*> clients;

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
							if (game.isGameOver())
							{
								clients.Empty();				
							}
							Client* client = new Client();
							memcpy(client->ip, ip, 4);
							client->playerNum = clients.size;
							connections[ip[3]] = client;
							clients.Push(client);
							game.Connect();
							puts("STATE is ");
							putdec(game.state);
							puts("\n");
							puts("Received a new connection request ");
							putdec(ip[3]);
							puts("\n");
							if(clients.size == 2)
							{
								game.Start();
							}
						}
					}
					break;

				case 'w':
				{
					if ((! game.isPaddleFrozen()) && (connections[ip[3]] != nullptr) && (connections[ip[3]]->playerNum < 2))
					{
						paddlePositions[connections[ip[3]]->playerNum] -= 10;
						if ((paddlePositions[connections[ip[3]]->playerNum] - paddleHeight/2) < 0)
						{
							paddlePositions[connections[ip[3]]->playerNum] = paddleHeight/2;
						}
					}
				}
				break;
				case 's':
				{
					if ((! game.isPaddleFrozen()) && (connections[ip[3]] != nullptr) && (connections[ip[3]]->playerNum < 2))
					{
						paddlePositions[connections[ip[3]]->playerNum] += 10;
						if ((paddlePositions[connections[ip[3]]->playerNum] + paddleHeight/2) > height)
						{
							paddlePositions[connections[ip[3]]->playerNum] = height - paddleHeight/2;
						}
					}
				}
				break;
				}
			}
			
		}

		game.Update();

		if (! game.isGameOver())
		{

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
		}
		else
		{
			puts("STATE is ");
			putdec(game.state);
			puts("\n");
			puts("Sending game results\n");
			List<Client*>::ListNode* node = clients.GetHead();
			while (node != nullptr)
			{
				unsigned char data[4];
				data[0] = 'g'; data[1] = ':'; data[3] = 0;
				if (node->value->playerNum == game.getWinner())
				{
					data[2] = 'w';
				}
				else
				{
					data[2] = 'l';
				}
				WriteSocket(socketDescriptor, node->value->ip, (unsigned char*)data, 4, sendPort);
				connections[node->value->ip[3]] = nullptr;
				node = node->next;
			}
		}
		//const int keyPressCount = GetQueuedKeyPressCount();

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

#include "NetworkProcess.h"
#include "pit.h"
#include "socket.h"

Socket* NetworkProcess::portTable[64] = {nullptr};

NetworkProcess::NetworkProcess() :
	Process("network", nullptr)
{

}

//dispatch packets to destination processes;
//send packets bbbbbbbbbbbbbbbbbbbbbbbbbbbb;
long NetworkProcess::run()
{
	while(1)
	{
		while(!this->networkSending.isEmpty())
		{
			Process::disable();
			Packet* nextPacket = this->networkSending.removeHead();
			Process::enable();
			if(!Network::KernelNetwork->SendPacket(nextPacket))
			{
				nextPacket->sendTime = Pit::milliseconds() + 100;
				this->tryAgainQueue.addTail(nextPacket);
			}
			else
			{
				delete nextPacket;
			}
		}

		while(!this->tryAgainQueue.isEmpty() && this->tryAgainQueue.peekHead()->sendTime <= Pit::milliseconds())
		{
			Process::disable();
			this->networkSending.addTail(tryAgainQueue.removeHead());
			Process::enable();
		}

		while(!this->networkReceiving.isEmpty())
		{
			Process::disable();
			Packet* nextPacket = this->networkReceiving.removeHead();
			Process::enable();

			Debug::printf("port number is %d\n", nextPacket->port);
			NetworkProcess::portTable[nextPacket->port]->GetOwner()->queueReceivePacket(nextPacket);
		}

		yield();
	}
}

void NetworkProcess::WriteToSocket(Socket* s, const unsigned char destinationIP[4], unsigned char port, const unsigned char* const buffer, int bufferLength)
{
	Packet* p = new Packet(bufferLength);
	p->port = port;
	p->protocol = s->GetProtocol();
	p->type = PacketType::IPv4;
	memcpy(p->IP, destinationIP, 4);
	memcpy(p->data, buffer, bufferLength);

	if ((destinationIP[0] == 127) && (destinationIP[1] == 0) && (destinationIP[2] == 0) && (destinationIP[3] == 1))
	{
		if(NetworkProcess::portTable[p->port] != nullptr)
			this->QueueNetworkReceive(p);
	}
	else
	{
		this->QueueNetworkSend(p);
	}
}

void NetworkProcess::QueueNetworkSend(Packet* packet)
{
	Process::disable();
	this->networkSending.addTail(packet);
	Process::enable();
}

void NetworkProcess::QueueNetworkReceive(Packet* packet)
{
	Process::disable();
	this->networkReceiving.addTail(packet);
	Process::enable();
}

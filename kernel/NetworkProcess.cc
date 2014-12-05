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

			NetworkProcess::portTable[nextPacket->port]->GetOwner()->queueReceivePacket(nextPacket);
		}

		yield();
	}
}

void NetworkProcess::WriteToSocket(Socket* s, const unsigned char destinationIP[4], const unsigned char* const buffer, int bufferLength)
{
	Packet* p = new Packet(bufferLength);
	p->port = s->GetPort();
	//p->
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

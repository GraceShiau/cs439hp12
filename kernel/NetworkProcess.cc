#include "NetworkProcess.h"

NetworkProcess::NetworkProcess() :
	Process("network",nullptr)
{

}

//dispatch packets to destination processes;
//send packets bbbbbbbbbbbbbbbbbbbbbbbbbbbb;
long NetworkProcess::run()
{
	while(1)
	{
		Process::disable();
		SimpleQueue<Packet*> tryAgainQueue;
		while(!this->networkSending.isEmpty())
		{
			Packet* nextPacket = this->networkSending.removeHead();
			if(!Network::KernelNetwork->SendPacket(nextPacket))
			{
				tryAgainQueue.addTail(nextPacket);
			}
			else
			{
				delete nextPacket;
			}
		}

		Process::sleepFor(500);

		while(!tryAgainQueue.isEmpty())
		{
			this->networkSending.addTail(tryAgainQueue.removeHead());
		}

		Process::enable();
		yield();
	}
}

void NetworkProcess::QueueNetworkSend(Packet* packet)
{
	this->networkSending.addTail(packet);
}

void NetworkProcess::QueueNetworkReceive(Packet* packet)
{
	this->networkReceiving.addTail(packet);
}

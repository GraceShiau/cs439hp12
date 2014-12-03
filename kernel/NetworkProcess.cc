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
		while(!this->networkSending.isEmpty())
		{
			Packet* nextPacket = this->networkSending.removeHead();
			Network::KernelNetwork->SendPacket(nextPacket);
			delete nextPacket;
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

#ifndef _NETWORK_PROCESS_H_
#define _NETWORK_PROCESS_H_
#include "process.h"
#include "network.h"

class Socket;

class NetworkProcess : public Process
{
public:
	static constexpr int portCount = 64;

	static Socket* portTable[portCount]; //indexed by port number;

	SimpleQueue<Packet*> networkSending;
	SimpleQueue<Packet*> networkReceiving;
	SimpleQueue<Packet*> tryAgainQueue;

	NetworkProcess();
    virtual long run();

    void QueueNetworkSend(Packet* packet);
    void QueueNetworkReceive(Packet* packet);
    void WriteToSocket(Socket* s, const unsigned char destinationIP[4], const unsigned char* const buffer, int bufferLength);
};


#endif

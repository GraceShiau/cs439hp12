#ifndef _SOCKET_H_H_
#define _SOCKET_H_H_
#include "resource.h"
#include "network.h"

class Process;

class Socket : public Resource
{
	const PacketProtocol protocol;
	const int port;
	Process* owner;

public:
	Socket(PacketProtocol protocol, char port, Process* owner);

	inline PacketProtocol GetProtocol() const {return this->protocol;}
	inline int GetPort() const {return this->port;}
	inline Process* GetOwner() const {return this->owner;}
};





#endif

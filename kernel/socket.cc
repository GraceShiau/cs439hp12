#include "socket.h"

Socket::Socket(PacketProtocol protocol, int port, Process* owner) :
	Resource(ResourceType::SOCKET),
	protocol(protocol),
	port(port),
	owner(owner)
{

}

#ifndef _NETWORK_H_
#define _NETWORK_H_

#include "debug.h"

enum class PacketType
{
	UNKNOWN,
	ARP,
	IPv4,
	IPv6
};

enum class PacketProtocol
{
	ICMP,
	P439
};

struct Packet
{
	PacketType type;
	PacketProtocol protocol;
	unsigned long length;
	unsigned char* data;
	bool isReply;
	unsigned char IP[4];

	Packet(unsigned long length) :
		length(length),
		data(new unsigned char[length]),
		isReply(false)
	{

	}

	~Packet()
	{
		delete[] data;
	}
};


struct ARPEntry
{
	unsigned char ipAddress[4];
	unsigned char macAddress[6];
};

struct ARPPacket
{
	unsigned char hardwareType[2];
	unsigned char protocolType[2];
	unsigned char hardwareLen;
	unsigned char protocolLen;
	unsigned char operationCode[2];
	unsigned char srcMac[6];
	unsigned char srcIP[4];
	unsigned char destMac[6];
	unsigned char destIP[4];

	void printPacket() {
		Debug::printf(" Hardware type is %x%x\n", hardwareType[0], hardwareType[1]);
		Debug::printf(" Protocol type is %x%x\n", protocolType[0], protocolType[1]);
		Debug::printf(" MAC length is %d IP length is %d\n", hardwareLen, protocolLen);
		Debug::printf(" Operation code is %x%x\n", operationCode[0], operationCode[1]);
		Debug::printf(" Source MAC is ");
		for (int i = 0; i < 5; ++i)
			Debug::printf("%x:", srcMac[i]);
		Debug::printf("%x\n", srcMac[5]);
		Debug::printf(" Source IP is ");
		for (int i = 0; i < 3; ++i)
			Debug::printf("%d:", srcIP[i]);
		Debug::printf("%d\n", srcIP[3]);
		Debug::printf(" Target MAC is ");
		for (int i = 0; i < 5; ++i)
			Debug::printf("%x:", destMac[i]);
		Debug::printf("%x\n", destMac[5]);
		Debug::printf(" Target IP is ");
		for (int i = 0; i < 3; ++i)
			Debug::printf("%d:", destIP[i]);
		Debug::printf("%d\n\n", destIP[3]);
	}
};

class ARPCache
{
public:
	void AddEntry(const unsigned char ipAddress[4], const unsigned char macAddress[6]);
	bool GetEntry(const unsigned char ipAddress[4], unsigned char macAddress[6]) const;
private:
	ARPEntry cache[100];
	int count;
};

struct IPv4Header
{
	unsigned char versionIHL;
	unsigned char DSCPECN;
	unsigned char totalLength[2];
	unsigned char identification[2];
	unsigned char flagsFOffst[2];
	unsigned char timeToLive;
	unsigned char protocol;
	unsigned char headerChecksum[2];
	unsigned char srcIPAddress[4];
	unsigned char destIPAddress[4];

	inline void print()
	{
		Debug::printf("IPv4header:\n Total length: %x%x\n", totalLength[0], totalLength[1]);
		Debug::printf(" Protocol: %x\n", protocol);
		Debug::printf(" Source IP: %d.%d.%d.%d\n", srcIPAddress[0], srcIPAddress[1], srcIPAddress[2], srcIPAddress[3]);
		Debug::printf(" Destination IP: %d.%d.%d.%d\n", destIPAddress[0], destIPAddress[1], destIPAddress[2], destIPAddress[3]);
	}

	IPv4Header() : 
		versionIHL(0x45),
		DSCPECN(0x00),
		totalLength({0x00, 0x54}),
		identification({0xfe, 0x8d}),
		flagsFOffst({0x40, 0x00}),
		timeToLive(0x40),
		protocol(),
		headerChecksum({0x00, 0x00}),
		srcIPAddress(),
		destIPAddress()
	{}

};

struct ICMPHeader
{
	unsigned char type;
	unsigned char code;
	unsigned char checksum[2];
	unsigned char identifier[2];
	unsigned char seqNum[2];

	inline void print()
	{
		Debug::printf("ICMP Header:\n");
		Debug::printf(" Type: %d\n", type);
		Debug::printf(" Code: %d\n", code);
	}

	ICMPHeader() :
		type(),
		code(),
		checksum({0x00, 0x00}),
		identifier({0x00, 0x00}),
		seqNum({0x00, 0x00})
	{}
};

class Network
{
public:
	static constexpr unsigned int networkBufferSize = 16000;
	static Network* KernelNetwork;
	static constexpr unsigned int ioaddr = 0xc000;
	static const unsigned char myMac[6];
	static const unsigned char myIP[4];
	ARPCache arpCache;
	long netCount;
	long currentBufferPosition;
	int currentBufferIndex;

	unsigned char ReceiveBuffer[Network::networkBufferSize];
	unsigned char TransmitBuffer[Network::networkBufferSize];

	Network();
	void Init();
	void HandleNetworkInterrupt();
	bool SendPacket(Packet* packet);

private:
	void ping(Packet *packet, const unsigned char destMac[6]);
	unsigned short getCurrentPacketLength() const;
	void endRxOKInterrupt();
	void endTxOKInterrupt();
	void getCurrentPacketSender(unsigned char sender[6]);
	PacketType getCurrentPacketType() const;
	void sendPacket(const unsigned char* data, int length);
	void sendARPRequest(const unsigned char ip[4]);
	void handlePacketReceiveInterrupt();
	bool isCurrentPacketForUs() const;
	void resplondToEchoRequest();
	unsigned char* currentBuffer() {return this->ReceiveBuffer + this->currentBufferPosition;}
	void calcChecksum(unsigned char* header, int length, unsigned char checksum[2]);

public:
	static void InitNetwork();
	static unsigned int pciConfigReadWord(unsigned char bus, unsigned char slot, unsigned
		char func, unsigned char offset);
	static void pciConfigWriteWord(unsigned char bus, unsigned char slot, unsigned
		char func, unsigned char offset, unsigned short val);
	static unsigned short pciCheckVendor(unsigned char bus, unsigned char slot);
};

#endif

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
	void AddEntry(const char ipAddress[4], const char macAddress[6]);
	ARPEntry GetEntry(const char ipAddress[4]) const;
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
	unsigned char sourceIPAddress[4];
	unsigned char destinationIPAddress[4];

	inline void print()
	{
		Debug::printf("IPv4header:\n Total length: %x%x\n", totalLength[0], totalLength[1]);
		Debug::printf(" Protocol: %x\n", protocol);
		Debug::printf(" Source IP: %d.%d.%d.%d\n", sourceIPAddress[0], sourceIPAddress[1], sourceIPAddress[2], sourceIPAddress[3]);
		Debug::printf(" Destination IP: %d.%d.%d.%d\n", destinationIPAddress[0], destinationIPAddress[1], destinationIPAddress[2], destinationIPAddress[3]);
	}
};

struct ICMPHeader
{
	unsigned char type;
	unsigned char code;
	unsigned char checksum[2];
	unsigned char restOfHeader[4];

	inline void print()
	{
		Debug::printf("ICMP Header:\n");
		Debug::printf(" Type: %d\n", type);
		Debug::printf(" Code: %d\n", code);
	}
};

class Network
{
public:
	static constexpr unsigned int networkBufferSize = 16000;
	static Network* KernelNetwork;
	static constexpr unsigned int ioaddr = 0xc000;
	static const unsigned char myMac[6];
	ARPCache arpCache;
	long netCount;
	long currentBufferPosition;
	int currentBufferIndex;

	unsigned char ReceiveBuffer[Network::networkBufferSize];
	unsigned char TransmitBuffer[Network::networkBufferSize];

	Network();
	void Init();
	void HandleNetworkInterrupt();

private:
	unsigned short getCurrentPacketLength() const;
	void endRxOKInterrupt();
	void endTxOKInterrupt();
	void getCurrentPacketSender(unsigned char sender[6]);
	PacketType getCurrentPacketType() const;
	void sendPacket(const unsigned char* data, int length);
	void handlePacketReceiveInterrupt();
	bool isCurrentPacketForUs() const;
	void resplondToEchoRequest();
	unsigned char* currentBuffer() {return this->ReceiveBuffer + this->currentBufferPosition;}

public:
	static void InitNetwork();
	static unsigned int pciConfigReadWord(unsigned char bus, unsigned char slot, unsigned
		char func, unsigned char offset);
	static void pciConfigWriteWord(unsigned char bus, unsigned char slot, unsigned
		char func, unsigned char offset, unsigned short val);
	static unsigned short pciCheckVendor(unsigned char bus, unsigned char slot);
};

#endif

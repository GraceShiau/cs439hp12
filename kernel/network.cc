#include "network.h"
#include "machine.h"
#include "NetworkProcess.h"

Network* Network::KernelNetwork = nullptr;
unsigned char Network::myMac[6] = {0x52,0x54,0x00,0x12,0x34,0x56};
unsigned char Network::myIP[4] = {192,168,7,2};
int Network::debugLevel = 1;

bool hasBroke = false;
void Network::HandleNetworkInterrupt() 
{
    ++this->netCount;

	unsigned short interruptType = inw(ioaddr + 0x3E);
	if(!hasBroke)
	Debug::printf("Received interrupt of type: %x from the network card.\n", interruptType);

    
    if((interruptType & 1) > 0)
    {
        this->handlePacketReceiveInterrupt();
    }
    if((interruptType & 2) > 0)
    {
        Debug::printf("Network interrupt error: %d\n", interruptType);
    }
    if((interruptType & 4) > 0)
    {
        //handle transmission complete
        this->endTxOKInterrupt();
    }
    if((interruptType & 16) > 0)
    {
    	//buffer full.
    	if(!hasBroke)
    	Debug::printf("Our network buffer is full.\n");
    	outw(ioaddr + 0x38, 8192);
        this->endRxOKInterrupt();
        hasBroke = true;
    }
    if((interruptType & 64) > 0)
    {
        Debug::printf("FIFO is full.\n");
        this->endRxOKInterrupt();
    }
}

bool Network::SendPacket(Packet* packet)
{
	switch(packet->type)
	{
	case PacketType::ARP:
		{
			if(packet->isReply)
			{
				if(debugLevel > 0)
					Debug::printf("SENDING ARP REPLY\n");
				this->sendPacket(packet->data, 42);
			}
			else
			{
				this->sendARPRequest(packet->IP);
			}
		}
		break;
	case PacketType::IPv4:
		{
			switch(packet->protocol)
			{
			case PacketProtocol::ICMP:
			{
				if(packet->isReply)
				{
                    Debug::printf("sending echo reply\n");
					this->sendPacket(packet->data, packet->length);
				}
                else 
                {
                    unsigned char destMac[6];
                    if(!this->arpCache.GetEntry(packet->IP, destMac))
                    {
                        Packet* p = new Packet(42);
                        memcpy(p->IP, packet->IP, 4);
                        p->type = PacketType::ARP;
                        p->isReply = false;
                        Process::networkProcess->QueueNetworkSend(p);
                        return false;
                    }
                    else
                    {
                        this->ping(packet, destMac);
                    }
                }
			}
			break;
			case PacketProtocol::P439:
			{
				unsigned char destMac[6];
				if(!this->arpCache.GetEntry(packet->IP, destMac))
				{
					Packet* p = new Packet(42);
					memcpy(p->IP, packet->IP, 4);
					p->type = PacketType::ARP;
					p->isReply = false;
					Process::networkProcess->QueueNetworkSend(p);
					return false;
				}
                else
                {
                    this->sendP439Packet(packet, destMac);
                }
			}
			break;
			}
		}
	}

    return true;
}

void Network::ping(Packet *packet, const unsigned char destMac[6])
{
    memcpy(packet->data, destMac, 6);
    memcpy(packet->data + 6, myMac, 6);
    packet->data[12] = 0x08;
    packet->data[13] = 0x00;

    IPv4Header ipv4Header;
    ipv4Header.protocol = 0x01;
    memcpy(ipv4Header.srcIPAddress, myIP, 4);
    memcpy(ipv4Header.destIPAddress, packet->IP, 4);

    this->calcChecksum((unsigned char *) &ipv4Header, sizeof(IPv4Header), ipv4Header.headerChecksum);

    memcpy(packet->data + 14, &ipv4Header, sizeof(IPv4Header));

    ICMPHeader icmpHeader;
    icmpHeader.type = 0x08;
    icmpHeader.code = 0x00;
    //don't know if makes sense
    icmpHeader.identifier[0] = 0x10;
    icmpHeader.identifier[1] = 0x0d;
    icmpHeader.seqNum[0] = 0x00;
    icmpHeader.seqNum[1] = 0x01;

    memcpy(packet->data + 14 + sizeof(IPv4Header), &icmpHeader, sizeof(ICMPHeader));

    for (int i = 58; i < 98; ++i)
    {
        packet->data[i] = i;
    }

    this->calcChecksum((unsigned char *) packet->data + 34, 64, packet->data + 36 );

	if(debugLevel > 1)
		Debug::printf("PINGINGINGINGING");

    this->sendPacket(packet->data, 98);
}

void Network::sendP439Packet(Packet *packet, const unsigned char destMac[6])
{
    unsigned char* buffer = new unsigned char[packet->length + 35];
    memcpy(buffer, destMac, 6);
    memcpy(buffer + 6, myMac, 6);
    buffer[12] = 0x08;
    buffer[13] = 0x00;

    IPv4Header ipv4Header;
    ipv4Header.protocol = 0x02;
    ipv4Header.totalLength[1] = packet->length + 21;
    memcpy(ipv4Header.srcIPAddress, myIP, 4);
    memcpy(ipv4Header.destIPAddress, packet->IP, 4);

    this->calcChecksum((unsigned char *) &ipv4Header, sizeof(IPv4Header), ipv4Header.headerChecksum);

    memcpy(buffer + 14, &ipv4Header, sizeof(IPv4Header));

    memcpy(buffer + 34, &packet->port, 1);

    memcpy(buffer + 35, packet->data, packet->length);

    this->sendPacket(buffer, packet->length + 35);

    delete[] buffer;
}

Network::Network() :
	netCount(0),
	currentBufferPosition(0),
	currentBufferIndex(0)
{

}

void Network::handlePacketReceiveInterrupt()
{
	const unsigned char* const rcvBuffer = this->ReceiveBuffer + this->currentBufferPosition;
    const unsigned short packetLength = this->getCurrentPacketLength();
    const unsigned short realPacketLength = packetLength + 4;
    const PacketType etherType = getCurrentPacketType();
    if(this->isCurrentPacketForUs())    
    {
        unsigned char sender[6];
        this->getCurrentPacketSender(sender);

		if(debugLevel > 1)
		{
			Debug::printf("current packet length is %d\n", packetLength);
			Debug::printf("sender is ");
			for (int i = 0; i < 5; ++i) {
				Debug::printf("%x:", sender[i]);
			}
			Debug::printf("%x\n", sender[5]);
			Debug::printf("type is %x\n", etherType);
		}

        switch(etherType)
        {
            case PacketType::ARP:
            {

        		if(debugLevel > 0)
        			Debug::printf("ARP PACKET");
                ARPPacket request;
                memcpy(&request, &this->ReceiveBuffer[this->currentBufferPosition + 18], 28);
                request.printPacket();

                this->arpCache.AddEntry(request.srcIP, request.srcMac);

                unsigned char packet[42] = {0};
                memcpy(packet, sender, 6);
                memcpy(packet+6, myMac, 6);
                packet[12] = 0x08;
                packet[13] = 0x06;

                ARPPacket reply;
                memcpy(reply.hardwareType, request.hardwareType, 2);
                memcpy(reply.protocolType, request.protocolType, 2);
                reply.hardwareLen = request.hardwareLen;
                reply.protocolLen = request.protocolLen;
                reply.operationCode[0] = 0;
                reply.operationCode[1] = 2;
                memcpy(reply.srcMac, myMac, 6);
                memcpy(reply.srcIP, request.destIP, 4);
                memcpy(reply.destMac, request.srcMac, 6);
                memcpy(reply.destIP, request.srcIP, 4);

                //reply.printPacket();

                memcpy(packet+14, &reply, 28);
                //this->sendPacket(packet, 42);
                Packet* p = new Packet(42);
                memcpy(p->data, packet, 42);
                p->isReply = true;
                p->type = PacketType::ARP;
                Process::networkProcess->QueueNetworkSend(p);
                break;
            }

            case PacketType::IPv4:
            {
        		if(debugLevel > 0)
        			Debug::printf("IPV4 PACKET: ");
                IPv4Header ipv4Header;
                memcpy(&ipv4Header, rcvBuffer + 18, sizeof(IPv4Header));
                /*for(int a = 0; a < packetLength; ++a)
                {     
                    Debug::printf("%d: %02x (%03d) \n", a, rcvBuffer[a],
                    		rcvBuffer[a]);
                }*/
                //ipv4Header.print();

                switch(ipv4Header.protocol)
                {
					case 1: //ICMP
					{
						if(debugLevel > 0)
							Debug::printf(" type ICMP\n");
						ICMPHeader icmpHeader;
						memcpy(&icmpHeader, rcvBuffer + 18 + sizeof(IPv4Header), sizeof(ICMPHeader));
						//icmpHeader.print();
						switch(icmpHeader.type)
						{
						case 0:
						{
							if(debugLevel > 0)
								Debug::printf("Received ICMP echo reply.\n");
                            ipv4Header.print();
							break;
						}
						case 8:
						{
							if(debugLevel > 0)
								Debug::printf("Received ICMP echo request.\n");
                            /*unsigned char destMac[6];
                            this->arpCache.GetEntry(ipv4Header.srcIPAddress, destMac);
                            for (int i = 0; i<6; ++i)
                            {
                                Debug::printf("%02x", destMac[i]);
                            }
                            Debug::printf("\n");*/

							this->resplondToEchoRequest();
							break;
						}
						default:
							Debug::printf("Received unknown ICMP packet type %d.\n", icmpHeader.type);
							break;
						}
					}
					break;

					case 2: //P439
					{
						const int len = ipv4Header.totalLength[0] * 256 + ipv4Header.totalLength[1] - 21;
						Packet* p = new Packet(len);
						memcpy(p->data, rcvBuffer + 18 + sizeof(IPv4Header) + 1, len);
						p->port = rcvBuffer[38];
						p->protocol = PacketProtocol::P439;
						p->type = PacketType::IPv4;
                        memcpy(p->IP, ipv4Header.srcIPAddress, 4);
                        Debug::printf(" Type P439\n");

						Process::networkProcess->QueueNetworkReceive(p);
					}
					break;
                }
                break;
            }

            case PacketType::IPv6:
            {
            	Debug::printf("IPV6 PACKET");
                break;
            }

            case PacketType::UNKNOWN:
            {
            	Debug::printf("UNKNOWN PACKET TYPE.");
                break;
            }
        }
    }
    else
    {

		if(debugLevel > 1)
			Debug::printf("Found somebody else's packet: %x %x %x %x %x %x\n",
					rcvBuffer[4],
					rcvBuffer[5],
					rcvBuffer[6],
					rcvBuffer[7],
					rcvBuffer[8],
					rcvBuffer[9]);
    }

    this->currentBufferPosition+= realPacketLength;
    this->currentBufferPosition = (this->currentBufferPosition + 3) & ~0x3;
    this->currentBufferPosition %= 8192;
	if(debugLevel > 0)
		Debug::printf("\nCurrent network receive buffer position: %d\n", this->currentBufferPosition);
	 outw(ioaddr + 0x38,  this->currentBufferPosition - 0x10);
    this->endRxOKInterrupt();
}
void Network::sendPacket(const unsigned char* data, int length)
{
	//hd((unsigned long)data, (unsigned long)data +100);
    //while(1) {}
    memcpy((void*)this->TransmitBuffer, data, length);	//copy the packet into the buffer

    //set addr and size of tx buffer
    outl(ioaddr + 0x20 + 4 * currentBufferIndex,  (unsigned long)this->TransmitBuffer);
    outl(ioaddr + 0x10 + 4 * currentBufferIndex, length);

    currentBufferIndex++;
    currentBufferIndex %= 4;
}

void Network::resplondToEchoRequest()
{
	const int len = this->getCurrentPacketLength() - 4;
	Packet* p = new Packet(len);
	memcpy(p->data, this->currentBuffer() + 4, len);

	//switch dest and src.
	memcpy(p->data, this->currentBuffer() + 10, 6);
	memcpy(p->data + 6, this->currentBuffer() + 4, 6);

	//set type to response.
	p->data[14 + sizeof(IPv4Header)] = 0;

	//switch ipv4 dest and src.
	memcpy(p->data + 26, this->currentBuffer() + 30 + 4, 4);
	memcpy(p->data + 30, this->currentBuffer() + 26 + 4, 4);
    p->data[24] = 0;
    p->data[25] = 0;
    this->calcChecksum((unsigned char *) p->data + 14, sizeof(IPv4Header), p->data + 24);
    
    p->data[36] = 0;
    p->data[37] = 0;
    this->calcChecksum((unsigned char *) p->data + 34, 64, p->data + 36 );

	//this->sendPacket(buffer, len);
	//delete[] buffer;
	p->protocol = PacketProtocol::ICMP;
	p->type = PacketType::IPv4;
    p->isReply = true;
	Process::networkProcess->QueueNetworkSend(p);
}

void Network::sendARPRequest(const unsigned char ip[4])
{
	if(debugLevel > 0)
    Debug::printf("SENDING ARP REQUEST");
    ARPPacket request;
    
    unsigned char packet[42] = {0};
    
    for (int i = 0; i < 6; ++i)
    {
        packet[i] = 0xFF;
    }

    memcpy(packet+6, myMac, 6);
    packet[12] = 0x08;
    packet[13] = 0x06;

    request.hardwareType[0] = 0x00;
    request.hardwareType[1] = 0x01;
    request.protocolType[0] = 0x08;
    request.protocolType[1] = 0x00;
    request.hardwareLen = 0x06;
    request.protocolLen = 0x04;
    request.operationCode[0] = 0x00;
    request.operationCode[1] = 0x01;
    memcpy(request.srcMac, myMac, 6);
    memcpy(request.srcIP, myIP, 4);
    request.destMac[6] = {0};
    memcpy(request.destIP, ip, 4);

    memcpy(packet+14, &request, 28);
    this->sendPacket(packet, 42);

}

bool Network::isCurrentPacketForUs() const
{
    int i = 0;
    for (; i < 6; ++i) 
    {
        //Debug::printf("Checking rcv %x vs our %x\n", this->ReceiveBuffer[this->currentBufferPosition + i + 4], myMac[i]);
        if(this->ReceiveBuffer[this->currentBufferPosition + i + 4] != myMac[i])
        {      
            break;
        }
    }

    if(i == 6)
        return true;

    i = 0;
    for (; i < 6; ++i) 
    {
        //Debug::printf("Checking rcv %x vs broadcast %x\n", this->ReceiveBuffer[this->currentBufferPosition + i + 4], 0xff);
        if(this->ReceiveBuffer[this->currentBufferPosition + i + 4] != 0xFF)
        {
            break;
        }
    }

    return i == 6;
}

//Retrieves the length of the packet currently
//pointed to by bufferPosition.
unsigned short Network::getCurrentPacketLength() const
{
    return *((short *) (this->ReceiveBuffer + this->currentBufferPosition + 2));
}

//Retrieves the sender of the packet currently
//pointed to by bufferPosition.
void Network::getCurrentPacketSender(unsigned char sender[6])
{
    for (int i = 0; i < 6; ++i) 
    {
        sender[i] = this->ReceiveBuffer[this->currentBufferPosition + i + 10];
    }
}

//Retrieves the sender of the packet currently
//pointed to by bufferPosition.
PacketType Network::getCurrentPacketType() const
{
    const unsigned char firstByte = this->ReceiveBuffer[this->currentBufferPosition + 16];
    const unsigned char secondByte = this->ReceiveBuffer[this->currentBufferPosition + 17];
    const unsigned short type =  (firstByte << 8) + secondByte;

    switch(type)
    {
        case 0x0806:
            return PacketType::ARP;
        case 0x0800:
            return PacketType::IPv4;
        case 0x86dd:
            return PacketType::IPv6;
    }

    return PacketType::UNKNOWN;
}

void Network::endRxOKInterrupt()
{
	 // Interrupt Status - Clears the Rx OK bit, acknowledging a packet has been received,
     // and is now in rx_buffer
	outw(ioaddr + 0x3E, 0x1);
}

void Network::endTxOKInterrupt()
{
     // Interrupt Status - Clears the Tx OWN bit, acknowledging a packet has been transmitted
    outl(ioaddr + 0x10, 0x0);
    outw(ioaddr + 0x3E, 0x4);
}

void Network::Init()
{
	const unsigned short vendor = pciCheckVendor(0, 3);
	Debug::printf("The vendor id is %x\n", vendor);
	
	for(int a = 0; a < 64; a+=2)
	{
		const unsigned short BAR0 = pciConfigReadWord(0, 3, 0, a);
	
		 Debug::printf("The thing at %x  is %x\n", a, BAR0);
	}	
	pciConfigWriteWord(0, 3, 0, 0xa, 6);
    outb( ioaddr + 0x52, 0x0);

    const long mac = inl(ioaddr);
    const long mac2 = inw(ioaddr + 4);
    Debug::printf("Found mac0-5 %x %x\n", mac, mac2);

    if (mac2 != 0x5634) {
        myIP[3] = 4;
        myMac[5] = 0x65;
        Debug::printf("Changing address");
    }

    if(mac == -1)
    {
    	return;
    }

    outb( ioaddr + 0x37, 0x10);
    while( (inb(ioaddr + 0x37) & 0x10) != 0) { }

    const long transmitStatus = inl(ioaddr + 0x10);
	Debug::printf("Transmit status: %x\n", transmitStatus);

	const long transmitBufferAddress = inl(ioaddr + 0x20);
    outl(ioaddr + 0x20, (long)TransmitBuffer);
    const long transmitBufferAddress2 = inl(ioaddr + 0x20);
    Debug::printf("Transmit buffer: %x, then after setting: %x\n", transmitBufferAddress, transmitBufferAddress2);

    const long receiveBufferAddress = inl(ioaddr + 0x30);
    outl(ioaddr + 0x30, (long)ReceiveBuffer);
    const long receiveBufferAddress2 = inl(ioaddr + 0x30);
    Debug::printf("Receive buffer: %x, then after setting: %x\n", receiveBufferAddress, receiveBufferAddress2);

    const long imrMask = inw(ioaddr + 0x3C);
    outw(ioaddr + 0x3C, 0xFFFF);
    const long imrMask2 = inw(ioaddr + 0x3C);
    Debug::printf("IMR mask: %x, then after setting: %x\n", imrMask, imrMask2);

    const long isrMask = inw(ioaddr + 0x3e);
    //outw(ioaddr + 0x3E, 0x0005);
    const long isrMask2 = inw(ioaddr + 0x3E);
    Debug::printf("ISR mask: %x, then after setting: %x\n", isrMask, isrMask2);

    const long rcvConfig =  0xf | (1 << 7);
    const long rcvConfigInitial = inl(ioaddr + 0x44);
    outl(ioaddr + 0x44,rcvConfig); // (1 << 7) is the WRAP bit, 0xf is AB+AM+APM+AAP
    const long rcvConfigSet = inl(ioaddr + 0x44);

    Debug::printf("Receive Config: %x, then after setting: %x\n", rcvConfigInitial, rcvConfigSet);


    const long reAndTe = inb(ioaddr + 0x37);
    outb(ioaddr + 0x37, 0x0C);
    const long reAndTe2 = inb(ioaddr + 0x37);
    Debug::printf("Receive / Transmit enable: %x, then after setting: %x\n", reAndTe, reAndTe2);

}

void Network::InitNetwork()
{
	Network::KernelNetwork = new Network();
	Network::KernelNetwork->Init();
	Process::networkProcess = new NetworkProcess();
    Process::networkProcess->start();
}

bool Network::CheckIP(const unsigned char ip[4])
{
    for (int i = 0; i < 4; ++i)
    {
        if (ip[i] != myIP[i])
        {
            return false;
        }
    }
    return true;
}

unsigned int Network::pciConfigReadWord(unsigned char bus, unsigned char slot, unsigned
		char func, unsigned char offset)
{
	unsigned int address;
	unsigned int lbus = (unsigned int)bus;
	unsigned int lslot = (unsigned int)slot;
	unsigned int lfunc = (unsigned int)func;
	unsigned short  tmp = 0;

	address = (unsigned int)((lbus << 16) | (lslot << 11) | (lfunc << 8) |
			(offset & 0xfc) | ((unsigned int)0x80000000));

	outl(0xCF8, address);

	//tmp = inl(0xfc);
	tmp = (unsigned short)((inl(0xcfc) >> ((offset & 2) * 8)) & 0xFFFF);
	return tmp;
}

void Network::pciConfigWriteWord(unsigned char bus, unsigned char slot, unsigned
		char func, unsigned char offset, unsigned short val)
{
	offset = 0x6;
	const int sz = 0;
	unsigned int address;
	unsigned int lbus = (unsigned int)bus;
	unsigned int lslot = (unsigned int)slot;
	unsigned int lfunc = (unsigned int)func;
	unsigned long  tmp = 0;

	address = (unsigned int)((lbus << 16) | (lslot << 11) | (lfunc << 8) |
			(offset & 0xfc) | ((unsigned int)0x80000000));

	//address = (lbus << 16) | (lslot << 11) | (lfunc << 8) | offset | 0x80000000;
	outl(0xCF8, address);

	tmp = inl(0xcfc + sz);
	Debug::printf("Read in %x\n", tmp);
	//tmp = (unsigned short)((inl(0xcfc) >> ((offset & 2) * 8)) & 0xFFFF);
	//return tmp;

	outl(0xcfc + sz, (unsigned int)0xFFFF0107);

	tmp = inl(0xcfc + sz);
	Debug::printf("Read in %x\n", tmp);
}

unsigned short Network::pciCheckVendor(unsigned char bus, unsigned char slot)
{
	unsigned short vendor;// device;

	if((vendor = pciConfigReadWord(bus, slot, 0, 0)) != 0xFFFF)
	{
	}

	return vendor;
}

void ARPCache::AddEntry(const unsigned char ipAddress[4], const unsigned char macAddress[6])
{
    memcpy(this->cache[count].ipAddress, ipAddress, 4);
    memcpy(this->cache[count].macAddress, macAddress, 6);
    count = (count + 1) % 100;
}

bool ARPCache::GetEntry(const unsigned char ipAddress[4], unsigned char macAddress[6]) const
{
    for (int i = 0; i < 100; ++i)
    {
        bool f = true;
        for (int j = 0; j < 4; ++j)
        {
            if (ipAddress[j] != this->cache[i].ipAddress[j])
            {
                f = false;
                break;
            }
        }
        if (f)
        {
            memcpy(macAddress, this->cache[i].macAddress, 6);
            return true;
        }
    }
    return false;
}

void Network::calcChecksum(unsigned char* header, int length, unsigned char checksum[2])
{
    unsigned long sum = 0;

    while (length > 1)
    {
        sum += *((unsigned short *) header);
        header += 2;
        length -=2;
    }

    if (length > 0)
    {
        sum += *header;
    }

    if (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    sum = ~sum;
    checksum[0] = sum & 0xFF;
    checksum[1] = sum >> 8;  
}

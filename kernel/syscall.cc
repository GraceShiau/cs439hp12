#include "syscall.h"
#include "machine.h"
#include "idt.h"
#include "process.h"
#include "child.h"
#include "fs.h"
#include "err.h"
#include "u8250.h"
#include "libk.h"
#include "screenbuffer.h"
#include "network.h"
#include "kbd.h"
#include "permission.h"
#include "NetworkProcess.h"
#include "socket.h"

void Syscall::init(void) {
    IDT::addTrapHandler(100,(uint32_t)syscallTrap,3);
}

// Helper methods for login authentication
bool foundUser(const char* buf, const char* username) {
	for (long i = 0; i < 10; i++) {
		char x = buf[i];
        if (x != username[i]) {
        	return false;
        }
        if (x == 0) {
        	return true;
		}
	}
	return true;
}

bool passwordMatches(const char* buf, const char* password) {
	for (long i = 10; i < 42; i++) {
		char x = buf[i];
		if (x != password[i - 10]) return false;
    }
	return true;
}

extern "C" long syscallHandler(uint32_t* context, long num, long a0, long a1) {

    switch (num) {
    case 0: /* exit */
        Process::exit(a0);
        return -1;
    case 1: /* putchar */
        Debug::printf("%c",a0);
        return 0;
    case 2: /* fork */
        {
            uint32_t userPC = context[8];
            uint32_t userESP = context[11];
            Child *child = new Child(Process::current);
            child->pc = userPC;
            child->esp = userESP;
            child->eax = 0;
            long id = Process::current->resources->open(child);
            child->start();

            return id;
        }
    case 3: /* semaphore */
        {
            Semaphore *s = new Semaphore(a0);
            return Process::current->resources->open(s);
        }
    case 4: /* down */
        {
            Semaphore* s = (Semaphore*) Process::current->resources->get(
                 a0,ResourceType::SEMAPHORE);
            if (s == nullptr) return ERR_INVALID_ID;
            s->down();
            return 0;
        }
    case 5 : /* up */
        {
            Semaphore* s = (Semaphore*) Process::current->resources->get(a0,
                    ResourceType::SEMAPHORE);
            if (s == nullptr) return ERR_INVALID_ID;
            s->up();
            return 0;
        }
    case 6 : /* join */
        {
            Process *proc = (Process*) Process::current->resources->get(a0,
                 ResourceType::PROCESS);
            if (proc == nullptr) return ERR_INVALID_ID;
            proc->doneEvent.wait();
            long code = proc->exitCode;
            Process::current->resources->close(a0);
            return code;
        }
    case 7 : /* shutdown */
        {
            Debug::shutdown("");
            return 0;
        }
    case 8 : /* open */
        {
            File* f = FileSystem::rootfs->rootdir->lookupFile((char*) a0);
            if (f == nullptr) return ERR_NOT_FOUND;
            bool access = Process::current->userPermissions->Access((const char*)a0, 0);
            if (!access) return ERR_ACCESS_DENIED;
            else return Process::current->resources->open(f);
        }
    case 9 : /* getlen */
        {
             File* f = (File*) Process::current->resources->get(a0,ResourceType::FILE);
             if (f == nullptr) {
                 return ERR_INVALID_ID;
             }
             return f->getLength();
        }
    case 10: /* read */
        {
             long *args = (long*) a0;
             File* f = (File*) Process::current->resources->get(args[0],ResourceType::FILE);
             if (f == nullptr) {
                 return ERR_INVALID_ID;
             }
             void* buf = (void*) args[1];
             long len = (long) args[2];
             return f->read(buf,len);
        }
    case 11 : /* seek */
        {
             File* f = (File*) Process::current->resources->get(a0,ResourceType::FILE);
             if (f == nullptr) {
                 return ERR_INVALID_ID;
             }
             f->seek(a1);
             return 0;
        }
    case 12 : /* close */
        {
             return Process::current->resources->close(a0);
        }
    case 13: /* execv */
        {
             /* find the security exposures in this code */
             char* name = (char*) a0;
             char** userArgs = (char**) a1;
             SimpleQueue<const char*> args;

             int i = 0;

             while(true) {
                 char* s = K::strdup(userArgs[i]);
                 if (s == 0) break;
                 args.addTail(s);
                 i++;
             }
             long rc = Process::current->execv(name,&args,i,true);

             /* execv failed, cleanup */
             while (!args.isEmpty()) {
                 const char* s = args.removeHead();
                 delete[] s;
             }
             return rc;
        }
    case 14: /* getchar */
        {
              return U8250::it->get();
        }
    case 15: //test draw
	{
	   Network::InitNetwork();
	}
	return 0;
	
	case 16: //get screen buffer(int width, int height)
	{
		//we have to know whether there is already a process controlling the global buffer.
		//we need to fool subsequent programs into thinking they have access to a real buffer.

		if(ScreenBuffer::globalBuffer == nullptr)
		{
			ScreenBuffer* screenBuffer = new ScreenBuffer(320, 200, Process::current->getId());
			const long resourceId = Process::current->resources->open(screenBuffer);
			ScreenBuffer::globalBuffer = screenBuffer;

			return resourceId;
		}
		else
		{
			ScreenBuffer* globalSreenBuffer = ScreenBuffer::globalBuffer;
			globalSreenBuffer->Lock();
			const int width = a0;
			const int height = a1;
			unsigned char* newBuffer = new unsigned char[width * height];
			ScreenBuffer* screenBuffer = new ScreenBuffer(width, height, Process::current->getId(), reinterpret_cast<unsigned int>(newBuffer));
			const long resourceId = Process::current->resources->open(screenBuffer);
			globalSreenBuffer->AddBufferRequest(screenBuffer);
			globalSreenBuffer->Unlock();
			return resourceId;
		}
	}
	case 17: //write screen buffer
	{
		ScreenBuffer* screenBuffer = static_cast<ScreenBuffer*>(Process::current->resources->get(a0, SCREEN_BUFFER));
		if(screenBuffer == nullptr)
		{
			return -1;
		}

		const unsigned char* buf = (const unsigned char*)(a1);
		screenBuffer->WriteBuffer(buf);
	}
	return 0;

	case 18: //GetNewWindowRequests()
	{
		//we would like to return to the requesting process a list of new processes requesting windows so that
		//the requesting process can do things for them.
		//we only want to grant this privilege to the process owning the global screen buffer.

		//We also include the width and height of the new window in this array when we return.
		ScreenBuffer* screenBuffer = ScreenBuffer::globalBuffer;
		if(screenBuffer == nullptr || screenBuffer->GetOwnerProcessId() != Process::current->getId())
		{
			Debug::printf("Fail\n");
			return -1;
		}

		const int requestCount = screenBuffer->GetBufferRequestCount();

		if(requestCount < 1)
		{
			return 0;
		}

		int* processIds = reinterpret_cast<int*>(a0);

		for(int a = 0; a < requestCount * 3; a += 3)
		{
			screenBuffer->GetNextChildBuffer(processIds[a * 3], processIds[a * 3 + 1], processIds[a * 3 + 2]);
		}
	}

	return 0;

	case 19: //GetBufferRequestCount()
	{
		ScreenBuffer* screenBuffer = ScreenBuffer::globalBuffer;
		if(screenBuffer == nullptr || screenBuffer->GetOwnerProcessId() != Process::current->getId())
		{
			return -1;
		}

		return screenBuffer->GetBufferRequestCount();
	}

	case 20: //GetChildBuffer(unsigned char* buf, int processId)
	{
		//Debug::printf("About to get child buffer to draw from here.\n");
		ScreenBuffer* screenBuffer = ScreenBuffer::globalBuffer;
		if(screenBuffer == nullptr || screenBuffer->GetOwnerProcessId() != Process::current->getId())
		{
			return -1;
		}

		unsigned char* buf = (unsigned char*)a0;
		const int processId = a1;

		ScreenBuffer* childBuffer = screenBuffer->GetChildBuffer(processId);

		childBuffer->Lock();
		if(childBuffer == nullptr)
		{
			Debug::printf("Error, buffer for process %d is null.\n", processId);
		}
		const int childBufferSize = childBuffer->GetWidth() * childBuffer->GetHeight();

		memcpy(buf, childBuffer->GetBuffer(), childBufferSize);

		childBuffer->Unlock();
		return 0;
	}
	case 21: //LockScreenBuffer(int id)
	{
		ScreenBuffer* screenBuffer = static_cast<ScreenBuffer*>(Process::current->resources->get(a0, SCREEN_BUFFER));
		if(screenBuffer == nullptr)
		{
			return -1;
		}

		screenBuffer->Lock();
	}
	return 0;
	case 22: //LockScreenBuffer()
	{
		ScreenBuffer* screenBuffer = static_cast<ScreenBuffer*>(Process::current->resources->get(a0, SCREEN_BUFFER));
		if(screenBuffer == nullptr)
		{
			return -1;
		}

		screenBuffer->Unlock();
	}
	return 0;

	case 23: //wait(int milliseconds)
	{
		Process::current->sleepFor(a0);
	}
	return 0;
	case 24: //ping(unsigned char ip[4])
	{
        Packet* packet = new Packet(98);
        packet->type = PacketType::IPv4;
        packet->protocol = PacketProtocol::ICMP;
        memcpy(packet->IP, (unsigned char *) a0, 4);
        Process::networkProcess->QueueNetworkSend(packet);
	}
	return 0;
	case 25: //long GetKeyPresses(char* buf, int bufferLength);
	{
		char* buf = (char*)a0;
		const int bufferLength = a1;
		int bytesRead = 0;
		const int count = ((BB<char>*)(Keyboard::is))->GetCount();
		for(bytesRead = 0; bytesRead < bufferLength && bytesRead < count; ++bytesRead)
		{
			buf[bytesRead] = Keyboard::is->get();
		}

		return bytesRead;
	}
	return 0;
	case 26: //QueueChildKeyInput(long childId, char key)
	{
		Process::processList[a0]->queueKeyPress(a1);
	}
	return 0;
	case 27: //GetQueuedkeyPressCount()
	{
		return Process::current->getKeyPressCount();
	}
	case 28: //GetQueuedKeyPresses(char* buf, int len);
	{
		int foundCount = 0;
		char* buf = (char*)a0;
		for(foundCount = 0; foundCount < a1 && !Process::current->keyQueue.isEmpty(); ++foundCount)
		{
			buf[foundCount] = Process::current->keyQueue.removeHead();
			++foundCount;
		}

		return foundCount;
	}
	case 29: //CheckLoginCred(char* userName, char* hash)
	{
    	File* f = FileSystem::rootfs->rootdir->lookupFile("passwords");
    	char buf[42];
    	uint32_t len = f->getLength();
		for (long i = 0; i < len; i+=42) {
			f->readFully(buf, 42);
			if (foundUser(buf, (const char*) a0)) {
				if (passwordMatches(buf, (const char*) a1)) {
					long userID = i / 42;
					Process::userPermissions = new Permission(userID);	 // set user permission
					delete f;
					return userID;
				}
			}
		}
		delete f;
		return -1;
	}
	case 30: //OpenSocket(int protocol, int port)
	{
		Socket* socket = new Socket(static_cast<PacketProtocol>(a0), a1, Process::current);
		const long resourceId = Process::current->resources->open(socket);
		NetworkProcess::portTable[a1] = socket;

		return resourceId;
	}
	case 31: //ReadSocket(long socketDescriptor, unsigned char srcIP[4], unsigned char* buffer,  long bufferSize);
	{
		//Debug::printf("Attempting to read buffersize: %d\n", reinterpret_cast<int*>(a0)[3]);
		const int socketDescriptor = reinterpret_cast<int*>(a0)[0];
		Socket* socket = static_cast<Socket*>(Process::current->resources->get(socketDescriptor, SOCKET));
		if(socket == nullptr)
		{
			return -1;
		}

		Packet* readPacket = Process::current->getNextQueuedPacket();
		if(readPacket == nullptr || readPacket->port != socket->GetPort() || readPacket->protocol != socket->GetProtocol())
		{
			return 0;
		}

		//Debug::printf("Read one packet\n");

		const int bufferSize = reinterpret_cast<int*>(a0)[3];
		if(readPacket->length > bufferSize)
		{
			return -2;
		}

		unsigned char* srcIP = reinterpret_cast<unsigned char*>(reinterpret_cast<int*>(a0)[1]);
		memcpy(srcIP, readPacket->IP, 4);

		unsigned char* buffer = reinterpret_cast<unsigned char*>(reinterpret_cast<int*>(a0)[2]);

		const int packetLength = readPacket->length;
		memcpy(buffer, readPacket->data, packetLength);
		delete readPacket;

		return packetLength;
	}
	return 0;
	case 32: //WriteSocket(long socketDescriptor, const unsigned char destinationIP[4], const unsigned char* const buffer, long bufferSize, unsigned char port);
	{
		//Debug::printf("Attempting to read buffersize: %d\n", reinterpret_cast<int*>(a0)[3]);
		const int socketDescriptor = reinterpret_cast<int*>(a0)[0];
		Socket* socket = static_cast<Socket*>(Process::current->resources->get(socketDescriptor, SOCKET));
		if(socket == nullptr)
		{
			return -1;
		}

		const unsigned char* destIP = reinterpret_cast<unsigned char*>(reinterpret_cast<int*>(a0)[1]);
		if (Network::CheckIP(destIP))
		{
			return -2;
		}
		//Debug::printf("Going to send a packet\n");
		const unsigned char* const buffer = reinterpret_cast<unsigned char*>(reinterpret_cast<int*>(a0)[2]);
		const int bufferSize = reinterpret_cast<int*>(a0)[3];
		const unsigned char port = reinterpret_cast<unsigned int*>(a0)[4];
		Process::networkProcess->WriteToSocket(socket, destIP, port, buffer, bufferSize);
	}
	return 0;
	case 33: //ToggleNetworkDebuggin(long truth)
	{

	}
	return 0;
	default:
        Process::trace("syscall(%d,%d,%d)",num,a0,a1);
        return -1;
    }
}

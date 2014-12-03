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

void Syscall::init(void) {
    IDT::addTrapHandler(100,(uint32_t)syscallTrap,3);
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
//
             SimpleQueue<const char*> args;

             int i = 0;

             while(true) {
                 char* s = K::strdup(userArgs[i]);
                 if (s == 0) break;
                 args.addTail(s);
                 i++;
             }

             long rc = Process::current->execv(name,&args,i, true);

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
	
	case 16: //get screen buffer
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
			const int width = 80;
			const int height = 60;
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

		int* processIds = (int*)a0;

		for(int a = 0; a < requestCount; ++a)
		{
			processIds[a] = screenBuffer->GetNextChildBuffer();
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
		Network::KernelNetwork->Ping((const unsigned char*)a0);
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
	case 29: //SetUserPermissions(long uid)
	{
		Process::userPermissions = new Permission(a0);
	}
	return 0;
    default:
        Process::trace("syscall(%d,%d,%d)",num,a0,a1);
        return -1;
    }
}

#include "screenbuffer.h"

ScreenBuffer* ScreenBuffer::globalBuffer = nullptr;

ScreenBuffer::ScreenBuffer(int width, int height, unsigned int ownerProcessId, unsigned int bufferLocation) :
		Resource(SCREEN_BUFFER),
		width(width),
		height(height),
		buffer((unsigned char*)bufferLocation),
		ownerProcessId(ownerProcessId)
{
	Debug::printf("Creating new screen buffer for process: %d with width: %d and height: %d.\n", this->ownerProcessId, this->width, this->height);
}

void ScreenBuffer::GetNextChildBuffer(int& processId, int& windowWidth, int& windowHeight)
{
	if(this->bufferRequests.isEmpty())
	{
		processId = -1;
		return;
	}

	ScreenBuffer*  nextBuffer = this->bufferRequests.removeHead();

	processId = nextBuffer->ownerProcessId;
	windowWidth = nextBuffer->width;
	windowHeight = nextBuffer->height;
	this->childBuffers.Push(nextBuffer);
}

 ScreenBuffer* ScreenBuffer::GetChildBuffer(int processId)
{
	//Debug::printf("About to find child buffers.\n");
	List<ScreenBuffer*>::ListNode* first = this->childBuffers.GetHead();
	if(first == nullptr)
		return nullptr;

	while(first != nullptr && first->value->ownerProcessId != processId)
	{
		first = first->next;
	}

	//Debug::printf("Found buffer %x with pid: %d.\n", (long)first->value, first->value->GetOwnerProcessId());
	return first->value;
}

ScreenBuffer::~ScreenBuffer()
{
	Debug::printf("DELETING SCREEN BUFFER!!\n\n\n\n!!!!!!!!!!");
	delete[] this->buffer;
}


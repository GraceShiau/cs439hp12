#ifndef _SCREENBUFFER_H_
#define _SCREENBUFFER_H_
#include "resource.h"
#include "machine.h"
#include "queue.h"
#include "semaphore.h"
//The buffer request resource keeps track of processes who request
//a window buffer when a former process already owns the rights to render it.
//the owning process of the global buffer should find these requests so that
//it may handle the itself.

class BufferRequest : public Resource
{
public:
	int processId;
	int resourceId;

	BufferRequest(int processId) : Resource(BUFFER_REQUEST), processId(processId), resourceId(-1) {}
};

class ScreenBuffer : public Resource
{
public:
	static ScreenBuffer* globalBuffer;

private:
	Mutex mutex;
	static constexpr unsigned int vgaMemory = 0xA0000;
	const int width;
	const int height;
	unsigned char* const buffer;
	const unsigned int ownerProcessId;
	SimpleQueue<ScreenBuffer*> bufferRequests; // a list of processes requesting to create a window
	List<ScreenBuffer*> childBuffers;

public:
	ScreenBuffer(int width, int height, unsigned int ownerProcessId, unsigned int bufferLocation = ScreenBuffer::vgaMemory);
	inline void AddBufferRequest(ScreenBuffer* request) {this->bufferRequests.addTail(request);}
	inline void WriteBuffer(const unsigned char* buffer) {memcpy(this->buffer, buffer, this->width * this->height);}
	inline void WriteBuffer(int x, int y, unsigned char val) {this->buffer[this->width * y + x] = val;}

	inline int GetWidth() const {return this->width;}
	inline int GetHeight() const {return this->height;}
	inline unsigned int GetOwnerProcessId() const {return this->ownerProcessId;}
	ScreenBuffer* GetChildBuffer(int processId);
	inline void CopyChildBuffer(int processId, unsigned char* buf) {const ScreenBuffer* const child = this->GetChildBuffer(processId); memcpy(buf, child->buffer, child->width * child->height);}
	void GetNextChildBuffer(int& processId, int& windowWidth, int& windowHeight);
	inline int GetBufferRequestCount() {const int sz = this->bufferRequests.GetSize(); return sz;}
	inline unsigned char* GetBuffer() const {return this->buffer;}
	inline void Lock() {this->mutex.lock();}
	inline void Unlock() {this->mutex.unlock();}

	virtual ~ScreenBuffer();
};




#endif

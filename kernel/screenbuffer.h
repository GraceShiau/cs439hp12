#ifndef _SCREENBUFFER_H_
#define _SCREENBUFFER_H_
#include "resource.h"
#include "machine.h"

class ScreenBuffer : public Resource
{
private:
	static constexpr unsigned int vgaMemory = 0xA0000;
	const int width;
	const int height;
	unsigned char* const buffer;

public:
	ScreenBuffer(int width, int height);
	inline void WriteBuffer(const unsigned char* buffer) {memcpy(this->buffer, buffer, this->width * this->height);}
	inline void WriteBuffer(int x, int y, unsigned char val) {this->buffer[this->width * y + x] = val;}

	inline int GetWidth() const {return this->width;}
	inline int GetHeight() const {return this->height;}

};




#endif

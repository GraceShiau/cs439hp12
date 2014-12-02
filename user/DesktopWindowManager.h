#ifndef _TEST_PROGRAM_H_
#define _TEST_PROGRAM_H_

#include "libcc.h"

class ChildWindow
{
public:
	int xPosition;
	int yPosition;
	int width;
	int height;

	const int processId;

	int borderWidth;
public:
	ChildWindow(int xPosition, int yPosition, int width, int height, int processId, int borderWidth);
	inline int GetX() const {return this->xPosition;}
	inline int GetY() const {return this->yPosition;}
	inline int GetWidth() const {return this->width;}
	inline int GetHeight() const {return this->height;}
	inline int GetProcessId() const {return this->processId;}
	inline int GetBorderWidth() const {return this->borderWidth;}
};

class DesktopWindowManager
{
private:
	long screenBufferId;
	unsigned int width;
	unsigned int height;
	unsigned char* buffer;
	unsigned int testCount;

	int defaultChildX;
	int defaultChildY;

	int defaultChildWidth;
	int defaultChildHeight;

	int defaultBorderWidth;
	int backgroundBorderColor;
	int foregroundBorderColor;

	List<ChildWindow*> childWindows;
	List<ChildWindow*> windowRenderList; // index 0 is foregroundWindow
	ChildWindow* foregroundWindow;
public:
	DesktopWindowManager(unsigned int width, unsigned int height);
	void Initialize();
	void Run();

private:
	void renderChildren();
	void acquireNewChildProcesses();
	void sendBufferData() const;

	void renderWindowBorder(const ChildWindow* const window);
	void handleKeyInput();
};



#endif

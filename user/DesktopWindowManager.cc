/*
 * DesktopWindowManager.cc

 *
 *  Created on: Nov 25, 2014
 *      Author: vexal
 */
extern "C" {
#include "libc.h"
#include "sys.h"
}

#include "DesktopWindowManager.h"

DesktopWindowManager::DesktopWindowManager(int width, int height) :
	screenBufferId(-1),
	width(width),
	height(height),
	buffer(new unsigned char[width * height]),
	testCount(0),
	defaultChildX(30),
	defaultChildY(30),
	defaultChildWidth(80),
	defaultChildHeight(60),
	defaultBorderWidth(3),
	backgroundBorderColor(3),
	foregroundBorderColor(4),
	foregroundWindow(nullptr)
{

}

void DesktopWindowManager::Initialize()
{
	this->screenBufferId = GetScreenBuffer();
	if(this->screenBufferId < 0)
	{
		puts("Failed to acquire screen buffer.\n");
	}

	puts("Successfully acquired screen buffer.\n");
}

void DesktopWindowManager::Run()
{
	while(1)
	{
		if(this->foregroundWindow != nullptr)
			this->foregroundWindow = this->childWindows.GetHead()->value;
		this->handleKeyInput();
		LockScreenBuffer(this->screenBufferId);
		this->acquireNewChildProcesses();
		this->renderChildren();
		this->sendBufferData();
		UnlockScreenBuffer(this->screenBufferId);
		Sleep(5);
	}
}

//query the OS for a list of new child processes requesting a window since the last update
//maybe this can be replaced later with some sort of signal from OS to window manager process.
void DesktopWindowManager::acquireNewChildProcesses()
{
	const int newProcessCount = GetBufferRequestCount();

	if(newProcessCount > 0)
	{
		int* newProcessIds = new int[newProcessCount];
		GetNewWindowRequests(newProcessIds);
		for(int a = 0; a < newProcessCount; ++a)
		{
			puts("Initializing window for process: ");
			const int newId = newProcessIds[a];
			putdec(newId);
			puts("\n");
			ChildWindow* newWindow = new ChildWindow(this->defaultChildX, this->defaultChildY, 80, 60, newId, this->defaultBorderWidth);
			this->foregroundWindow = newWindow;
			this->defaultChildX += 15;
			this->defaultChildY += 15;
			this->childWindows.Push(newWindow);
		}

		delete[] newProcessIds;
	}
}

void DesktopWindowManager::renderChildren()
{
	List<ChildWindow*>::ListNode* last = this->childWindows.GetLast();

	for(int a = 0; a < this->width; ++a)
	{
		for(int b = 0; b < this->height; ++b)
		{
			this->buffer[b*this->width + a] = 0;
		}
	}

	while(last != nullptr)
	{
		//render the child window at the correct position.
		//render the children backwards.
		const ChildWindow* const child = last->value;
		this->renderWindowBorder(child);

		unsigned char* childWindowBuffer = new unsigned char[child->GetWidth() * child->GetHeight()];
		GetChildBuffer(childWindowBuffer, child->GetProcessId());
		int px = 0;

		for(int x = child->GetX(); x < child->GetX() + child->GetWidth(); ++x)
		{
			if(x >= this->width)
				continue;
			for(int y = child->GetY(); y < child->GetY() + child->GetHeight() && y < this->height; ++y)
			{
				const int pixelLocation = x + this->width * y;
				this->buffer[pixelLocation] = childWindowBuffer[px];

				++px;
			}
		}

		delete[] childWindowBuffer;
		last = last->previous;
	}
}

void DesktopWindowManager::handleKeyInput()
{
	char buf[128];
	const int keyPressCount = GetKeyPresses(buf, 128);

	for(int a = 0; a < keyPressCount; ++a)
	{
		const char kp = buf[a] & ~128;
		//puts("detected key press: ");
		char c[3] = {kp, '\n', 0};
		//puts(c);
		if((buf[a] & 128) > 0)
		{
			//puts("(key up)\n");
		}
		else
		{
			if(this->foregroundWindow != nullptr)
			{
				switch(kp)
				{
				case 'j':
					this->foregroundWindow->xPosition-= 1;
					break;
				case 'l':
					this->foregroundWindow->xPosition+= 1;
					break;
				case 'i':
					this->foregroundWindow->yPosition-= 1;
					break;
				case 'k':
					this->foregroundWindow->yPosition+= 1;
					break;
				case 'p':
					//puts("Previous foreground window is "); putdec(this->childWindows.GetHead()->value->GetProcessId()); puts("\n");
					this->childWindows.MoveFrontToEnd();
					//puts("New foreground window is "); putdec(this->childWindows.GetHead()->value->GetProcessId()); puts("\n");
					break;
				default:
					QueueChildKeyInput(this->foregroundWindow->GetProcessId(), kp);
					break;
				}
			}
		}
	}
}

void DesktopWindowManager::sendBufferData() const
{
	WriteScreenBuffer(screenBufferId, this->buffer);
}

void DesktopWindowManager::renderWindowBorder(const ChildWindow* const child)
{
	const int borderXLeftStart = child->GetX() - child->GetBorderWidth();
	for(int x = borderXLeftStart; x < borderXLeftStart + child->GetBorderWidth(); ++x)
	{
		if(x >= this->width)
			continue;

		for(int y = child->GetY() - child->GetBorderWidth(); y < child->GetY() + child->GetBorderWidth() + child->GetHeight() && y < this->height; ++y)
		{
			const int pixelLocation = x + this->width * y;
			this->buffer[pixelLocation] = this->foregroundWindow == child ? this->foregroundBorderColor : this->backgroundBorderColor;
		}
	}

	const int borderXRightStart = child->GetX() + child->GetWidth();
	for(int x = borderXRightStart; x < borderXRightStart + child->GetBorderWidth(); ++x)
	{
		if(x >= this->width)
			continue;

		for(int y = child->GetY() - child->GetBorderWidth(); y < child->GetY() + child->GetBorderWidth() + child->GetHeight() && y < this->height; ++y)
		{
			const int pixelLocation = x + this->width * y;
			this->buffer[pixelLocation] = this->foregroundWindow == child ? this->foregroundBorderColor : this->backgroundBorderColor;
		}
	}


	for(int x = borderXLeftStart; x < borderXLeftStart + child->GetWidth() + child->GetBorderWidth(); ++x)
	{
		if(x >= this->width)
			continue;

		for(int y = child->GetY() - child->GetBorderWidth(); y < child->GetY() + child->GetBorderWidth() && y < this->height; ++y)
		{
			const int pixelLocation = x + this->width * y;
			this->buffer[pixelLocation] = this->foregroundWindow == child ? this->foregroundBorderColor : this->backgroundBorderColor;
		}
	}

	for(int x = borderXLeftStart; x < borderXLeftStart + child->GetWidth() + child->GetBorderWidth(); ++x)
	{
		if(x >= this->width)
			continue;

		for(int y = child->GetY() + child->GetHeight(); y < child->GetY() + child->GetBorderWidth() + child->GetHeight() && y < this->height; ++y)
		{
			const int pixelLocation = x + this->width * y;
			this->buffer[pixelLocation] = this->foregroundWindow == child ? this->foregroundBorderColor : this->backgroundBorderColor;
		}
	}
}

ChildWindow::ChildWindow(int xPosition, int yPosition, int width, int height, int processId, int borderWidth) :
		xPosition(xPosition),
		yPosition(yPosition),
		width(width),
		height(height),
		processId(processId),
		borderWidth(borderWidth)
{

}




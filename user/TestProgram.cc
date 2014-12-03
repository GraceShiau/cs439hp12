extern "C" {
#include "libc.h"
#include "sys.h"
}

#include "DesktopWindowManager.h"

int main()
{
	testdraw('a');

	DesktopWindowManager windowManager(320, 200);
	windowManager.Initialize();
	windowManager.Run();
	return 0;
}

#ifndef _PERMISSION_H_
#define _PERMISSION_H_

#include "queue.h"

struct FilePermission
{
	bool readPermission;
	bool writePermission;
	bool executePermission;
};

class Permission {
	char* buffer;
	unsigned int len;
	long userId;
	Map<String, FilePermission*> filePermissions;
	// assume permissions file fits in one block
	void loadBuffer();
	void processBuffer();
public:
	Permission(long userID);
	// 0 for denied, 1 for granted
	bool Access(const char* fileName, unsigned int mode);
};

#endif

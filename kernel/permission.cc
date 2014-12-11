#include "permission.h"
#include "machine.h"
#include "fs.h"
#include "process.h"
#include "debug.h"

/*
 * Mode numbers: 0 read; 1 write; 2 execute.
 * UserID: order of appearance in both passwords and permissions files
 * No space or ";" allowed in file names
 * Each filename and permission entry must be followed by a space
 * Each user capability list is separated by a newline character
 */

	Permission::Permission(long userId) :
		userId(userId)
	{
		loadBuffer();
		processBuffer();
	}
	// assume permissions file fits in one block
	void Permission::loadBuffer() {
		// put the loaded buffer on heap and return the address
		int userNum = 0;
		char buf[512];
		int start = 0;
		int end;
		int seeking = 0;
		File* f = FileSystem::rootfs->rootdir->lookupFile("permissions");
		long length = f->readFully(buf, 512);
		while (userNum != userId + 1) {
			for (int i = seeking; i < length; i++) {
				if (buf[i] == '\n') {
					userNum++;
					if (userNum == userId) {
						start = i + 1;
					}
					if (userNum == userId + 1) {
						end = i + 1;
					}
					seeking = i + 1;
					break;
				}
			}
		}
		int len = end - start;
		this->len = len;
		this->buffer = new char[len + 1];
		memcpy(this->buffer, buf + start, len);
		buffer[len] = 0;
		delete f;
	}

	void Permission::processBuffer() {
		while(this->buffer[0] != '\n')
		{
			unsigned int index = 0;
			FilePermission *perm = new FilePermission();
			while(this->buffer[index] != ' ') {
				++index;
			}

			char* fileName = new char[index + 1];
			fileName[index] = 0;
			memcpy(fileName, this->buffer, index);
			this->filePermissions[fileName] = perm;
			const auto node = this->filePermissions.Find(this->filePermissions.root, fileName);
			buffer+= index + 1;

			perm->readPermission = buffer[0] == '1';
			perm->writePermission = buffer[1] == '1';
			perm->executePermission = buffer[2] == '1';
			buffer += 4;
		}
	}

	// 0 for denied, 1 for granted
	bool Permission::Access(const char* fileName, unsigned int mode) {
		const String fn = fileName;
		const auto node = this->filePermissions.Find(this->filePermissions.root, fn);
		if (node == nullptr) {
			return 0;
		}
		switch(mode) {
		case 0:
			return node->value->readPermission;
		case 1:
			return node->value->writePermission;
		case 2:
			return node->value->executePermission;
		}
	}

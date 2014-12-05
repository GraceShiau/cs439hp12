#ifndef _SYS_H_
#define _SYS_H_

extern long exit(long status);
extern long execv(char* prog, char** args);
extern long open(char *name);
extern long getlen(long);
extern long close(long);
extern long read(long f, void* buf, long len);
extern long seek(long f, long pos);
extern long putchar(int c);
extern long getchar();
extern long semaphore(long n);
extern long up(long sem);
extern long down(long sem);
extern long fork();
extern long join(long proc);
extern long shutdown();
extern long testdraw(long c);
extern long GetScreenBuffer();
extern long WriteScreenBuffer(long resourceId, unsigned char* buf);
extern long GetNewWindowRequests(int* buf);
extern long GetBufferRequestCount();
extern long GetChildBuffer(unsigned char* buf, int processId);
extern long LockScreenBuffer(long id);
extern long UnlockScreenBuffer(long id);
extern long Sleep(long milliseconds);
extern long Ping(const unsigned char destination[4]);
extern long GetKeyPresses(char* buf, int bufferLength);
extern long QueueChildKeyInput(long childId, char key);
extern long GetQueuedKeyPressCount();
extern long GetQueuedKeyPresses(char* buf, int len);
extern long CheckLoginCred(const char* userName, const char* hash);
extern long OpenSocket(int protocol, int port);
extern long ReadSocket(long socketDescriptor, unsigned char srcIP[4], unsigned char* buffer,  long bufferSize);
extern long WriteSocket(long socketDescriptor, const unsigned char destinationIP[4], const unsigned char* const buffer, long bufferSize);
#endif

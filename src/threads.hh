// threads.hh: Thread, the thread functions and Semaphore, a counting
// semaphore (threads.cc).

#ifndef PATELLA_THREADS_HH
#define PATELLA_THREADS_HH

#include <cstdint>
#include <pthread.h>
#include <stddef.h>

// The original is `typedef unsigned long Thread` and holds a pthread_t, which
// was 32 bits there. It has to hold a 64-bit pthread_t here.
typedef pthread_t Thread;

typedef int ThreadFunction(void *);

struct Semaphore {
	Semaphore(int initValue);
	~Semaphore();
	void down();
	void up();
private:
	int value;
	pthread_mutex_t mutex;
	pthread_cond_t condition;
};

void *ThreadStarter(void *Pointer);
Thread StartThread(ThreadFunction *Function, void *Argument, bool Detach);
Thread StartThread(ThreadFunction *Function, void *Argument, size_t StackSize, bool Detach);
Thread StartThread(ThreadFunction *Function, void *Argument, void *Stack, size_t StackSize, bool Detach);
void DelayThread(int Seconds, int MicroSeconds);
int JoinThread(Thread OtherThread);

#endif

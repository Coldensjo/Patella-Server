#include <cstdlib>
#include <stddef.h>

#include "threads.hh"
#include "utils.hh"

struct TThreadStarter {
	ThreadFunction *Function;
	void *Argument;
	bool Detach;
};

// 0x08068540  threads.c:31
void *ThreadStarter(void * /*Pointer*/) { abort(); /* STUB */ }

// 0x080685a0  threads.c:57
Thread StartThread(ThreadFunction * /*Function*/, void * /*Argument*/, bool /*Detach*/) { abort(); /* STUB */ }

// 0x08068630  threads.c:80
Thread StartThread(ThreadFunction * /*Function*/, void * /*Argument*/, size_t /*StackSize*/, bool /*Detach*/) { abort(); /* STUB */ }

// 0x080686e0  threads.c:109
Thread StartThread(ThreadFunction * /*Function*/, void * /*Argument*/, void * /*Stack*/, size_t /*StackSize*/, bool /*Detach*/) { abort(); /* STUB */ }

// 0x08068790  threads.c:136
void DelayThread(int /*Seconds*/, int /*MicroSeconds*/) { abort(); /* STUB */ }

// 0x080687d0  threads.c:153
int JoinThread(Thread /*OtherThread*/) { abort(); /* STUB */ }

// 0x08068870  threads.c:172
Semaphore::Semaphore(int /*initValue*/) { abort(); /* STUB */ }

// 0x08068920  threads.c:188
Semaphore::~Semaphore() { abort(); /* STUB */ }

// 0x08068970  threads.c:203
void Semaphore::down() { abort(); /* STUB */ }

// 0x080689e0  threads.c:214
void Semaphore::up() { abort(); /* STUB */ }

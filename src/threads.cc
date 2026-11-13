#include <cstdlib>
#include <pthread.h>
#include <sched.h>
#include <stddef.h>
#include <unistd.h>

#include "threads.hh"
#include "utils.hh"

struct TThreadStarter {
	ThreadFunction *Function;
	void *Argument;
	bool Detach;
};

// 0x08068540  threads.c:31
void *ThreadStarter(void *Pointer) {
	TThreadStarter *Starter = static_cast<TThreadStarter *>(Pointer);
	ThreadFunction *Function = Starter->Function;
	void *Argument = Starter->Argument;
	bool Detach = Starter->Detach;
	delete Starter;

	if (!Detach) {
		int *result = new int;
		*result = Function(Argument);
		pthread_exit(result);
	}

	Function(Argument);
	pthread_exit(nullptr);
}

// 0x080685a0  threads.c:57
Thread StartThread(ThreadFunction *Function, void *Argument, bool Detach) {
	TThreadStarter *Starter = new TThreadStarter;
	Starter->Function = Function;
	Starter->Argument = Argument;
	Starter->Detach = Detach;

	pthread_t thread;
	int result = pthread_create(&thread, nullptr, ThreadStarter, Starter);
	if (result != 0) {
		error("StartThread: Kann Thread nicht anlegen; Fehlercode %d.\n", result);
		return 0;
	}

	if (Detach) {
		pthread_detach(thread);
	}

	return thread;
}

// 0x08068630  threads.c:80
Thread StartThread(ThreadFunction *Function, void *Argument, size_t StackSize, bool Detach) {
	TThreadStarter *Starter = new TThreadStarter;
	Starter->Function = Function;
	Starter->Argument = Argument;
	Starter->Detach = Detach;

	pthread_attr_t Attributes;
	pthread_attr_init(&Attributes);
	pthread_attr_setstacksize(&Attributes, StackSize);

	pthread_t thread;
	int result = pthread_create(&thread, &Attributes, ThreadStarter, Starter);
	pthread_attr_destroy(&Attributes);
	if (result != 0) {
		error("StartThread: Kann Thread nicht anlegen; Fehlercode %d.\n", result);
		return 0;
	}

	if (Detach) {
		pthread_detach(thread);
	}

	return thread;
}

// 0x080686e0  threads.c:109
Thread StartThread(ThreadFunction *Function, void *Argument, void *Stack, size_t StackSize, bool Detach) {
	TThreadStarter *Starter = new TThreadStarter;
	Starter->Function = Function;
	Starter->Argument = Argument;
	Starter->Detach = Detach;

	pthread_attr_t Attributes;
	pthread_attr_init(&Attributes);
	pthread_attr_setstack(&Attributes, Stack, StackSize);

	pthread_t thread;
	int result = pthread_create(&thread, &Attributes, ThreadStarter, Starter);
	pthread_attr_destroy(&Attributes);
	if (result != 0) {
		error("StartThread: Kann Thread nicht anlegen; Fehlercode %d.\n", result);
		return 0;
	}

	if (Detach) {
		pthread_detach(thread);
	}

	return thread;
}

// 0x08068790  threads.c:136
void DelayThread(int Seconds, int MicroSeconds) {
	if (Seconds == 0 && MicroSeconds == 0) {
		sched_yield();
	} else if (MicroSeconds == 0) {
		sleep(Seconds);
	} else {
		usleep(Seconds * 1000000 + MicroSeconds);
	}
}

// 0x080687d0  threads.c:153
int JoinThread(Thread OtherThread) {
	int *result = nullptr;
	pthread_join(OtherThread, reinterpret_cast<void **>(&result));
	if (result == nullptr) {
		return 0;
	}

	int value = *result;
	delete result;
	return value;
}

// 0x08068870  threads.c:172
Semaphore::Semaphore(int initValue) {
	this->value = initValue;
	if (pthread_mutex_init(&this->mutex, nullptr) == -1) {
		error("Semaphore::Semaphore: Kann Mutex nicht einrichten.\n");
		return;
	}

	if (pthread_cond_init(&this->condition, nullptr) == -1) {
		error("Semaphore::Semaphore: Kann Wartebedingung nicht einrichten.\n");
	}
}

// 0x08068920  threads.c:188
Semaphore::~Semaphore() {
	if (pthread_mutex_destroy(&this->mutex) == -1) {
		error("Semaphore::~Semaphore: Kann Mutex nicht freigeben.\n");
		return;
	}

	if (pthread_cond_destroy(&this->condition) == -1) {
		error("Semaphore::~Semaphore: Kann Wartebedingung nicht freigeben.\n");
	}
}

// 0x08068970  threads.c:203
void Semaphore::down() {
	pthread_mutex_lock(&this->mutex);
	while (this->value <= 0) {
		pthread_cond_wait(&this->condition, &this->mutex);
	}
	this->value--;
	pthread_mutex_unlock(&this->mutex);
}

// 0x080689e0  threads.c:214
void Semaphore::up() {
	pthread_mutex_lock(&this->mutex);
	this->value++;
	pthread_mutex_unlock(&this->mutex);
	pthread_cond_signal(&this->condition);
}

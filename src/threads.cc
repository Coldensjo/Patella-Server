// threads.cc: threads on POSIX threads: starting one (with the default
// stack, a stack of a given size or a given stack), waiting for one and
// pausing the caller, and a counting semaphore.

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

Thread StartThread(ThreadFunction *Function, void *Argument, bool Detach) {
	TThreadStarter *Starter = new TThreadStarter;
	Starter->Function = Function;
	Starter->Argument = Argument;
	Starter->Detach = Detach;

	pthread_t thread;
	int result = pthread_create(&thread, nullptr, ThreadStarter, Starter);
	if (result != 0) {
		error("StartThread: Cannot create thread; error code %d.\n", result);
		return 0;
	}

	if (Detach) {
		pthread_detach(thread);
	}

	return thread;
}

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
		error("StartThread: Cannot create thread; error code %d.\n", result);
		return 0;
	}

	if (Detach) {
		pthread_detach(thread);
	}

	return thread;
}

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
		error("StartThread: Cannot create thread; error code %d.\n", result);
		return 0;
	}

	if (Detach) {
		pthread_detach(thread);
	}

	return thread;
}

void DelayThread(int Seconds, int MicroSeconds) {
	if (Seconds == 0 && MicroSeconds == 0) {
		sched_yield();
	} else if (MicroSeconds == 0) {
		sleep(Seconds);
	} else {
		usleep(Seconds * 1000000 + MicroSeconds);
	}
}

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

Semaphore::Semaphore(int initValue) {
	this->value = initValue;
	if (pthread_mutex_init(&this->mutex, nullptr) == -1) {
		error("Semaphore::Semaphore: Cannot set up mutex.\n");
		return;
	}

	if (pthread_cond_init(&this->condition, nullptr) == -1) {
		error("Semaphore::Semaphore: Cannot set up condition variable.\n");
	}
}

Semaphore::~Semaphore() {
	if (pthread_mutex_destroy(&this->mutex) == -1) {
		error("Semaphore::~Semaphore: Cannot free mutex.\n");
		return;
	}

	if (pthread_cond_destroy(&this->condition) == -1) {
		error("Semaphore::~Semaphore: Cannot free condition variable.\n");
	}
}

void Semaphore::down() {
	pthread_mutex_lock(&this->mutex);
	while (this->value <= 0) {
		pthread_cond_wait(&this->condition, &this->mutex);
	}
	this->value--;
	pthread_mutex_unlock(&this->mutex);
}

void Semaphore::up() {
	pthread_mutex_lock(&this->mutex);
	this->value++;
	pthread_mutex_unlock(&this->mutex);
	pthread_cond_signal(&this->condition);
}

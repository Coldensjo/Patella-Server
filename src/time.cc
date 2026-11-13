#include <cstdint>
#include <cstdlib>

#include "time.hh"

uint32_t RoundNr = 0;	// time.c:12
uint32_t ServerMilliseconds = 0;	// time.c:13

// 0x08053c00  time.c:19
void GetAmbiente(int & /*Brightness*/, int & /*Color*/) { abort(); /* STUB */ }

// 0x08053d10  time.c:43
void GetDate(int & /*Year*/, int & /*Cycle*/, int & /*Day*/) { abort(); /* STUB */ }

// 0x08053d90  time.c:56
void GetTime(int & /*Hour*/, int & /*Minute*/) { abort(); /* STUB */ }

// 0x08053e20  time.c:67
void GetRealTime(int & /*Hour*/, int & /*Minute*/) { abort(); /* STUB */ }

// 0x08053e60  time.c:78
uint32_t GetRoundAtTime(int /*Hour*/, int /*Minute*/) { abort(); /* STUB */ }

// 0x08053ed0  time.c:90
uint32_t GetRoundForNextMinute() { abort(); /* STUB */ }

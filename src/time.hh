// time.hh: the round counter, the server's milliseconds, and the game and
// real clocks (time.cc).

#ifndef PATELLA_TIME_HH
#define PATELLA_TIME_HH

#include <cstdint>

extern uint32_t RoundNr;
extern uint32_t ServerMilliseconds;

void GetAmbiente(int &Brightness, int &Color);
void GetDate(int &Year, int &Cycle, int &Day);
void GetTime(int &Hour, int &Minute);
void GetRealTime(int &Hour, int &Minute);
uint32_t GetRoundAtTime(int Hour, int Minute);
uint32_t GetRoundForNextMinute();

#endif

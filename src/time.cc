// time.cc: the round counter and the clocks: the game's time of day and its
// light, the game date, the real time of day, and the round at which a real
// time comes.

#include <cstdint>
#include <ctime>

#include "operate.hh"
#include "time.hh"

// The game clock runs 24 times as fast as the real one: a game hour lasts 150
// seconds, a game day one real hour (GetTime).
const int SECONDS_PER_GAME_HOUR = 150;

const int SECONDS_PER_DAY = 86400;

uint32_t RoundNr = 0;
uint32_t ServerMilliseconds = 0;

// Quirk: time() is narrowed to a signed 32-bit value wherever it is stored,
// as a 32-bit time_t holds it.

void GetAmbiente(int &Brightness, int &Color) {
	int Hour, Minute;

	GetTime(Hour, Minute);
	int Time = Hour * 60 + Minute;
	if (Time < 60) {
		Brightness = 51;
		Color = COLOR_WHITE;
	} else if (Time < 120) {
		Brightness = 102;
		Color = COLOR_WHITE;
	} else if (Time < 180) {
		Brightness = 153;
		Color = 173;
	} else if (Time < 240) {
		Brightness = 204;
		Color = 173;
	} else if (Time > 1380) {
		Brightness = 51;
		Color = COLOR_WHITE;
	} else if (Time > 1320) {
		Brightness = 102;
		Color = COLOR_WHITE;
	} else if (Time > 1260) {
		Brightness = 153;
		Color = 208;
	} else if (Time > 1200) {
		Brightness = 204;
		Color = 208;
	} else {
		Brightness = 255;
		Color = COLOR_WHITE;
	}
}

void GetDate(int &Year, int &Cycle, int &Day) {
	time_t t = static_cast<int32_t>(time(nullptr));
	struct tm *Time = localtime(&t);

	Year = (static_cast<int32_t>(t) / SECONDS_PER_DAY + 4) / 7;
	Cycle = Time->tm_wday;
	Day = Time->tm_hour;
}

void GetTime(int &Hour, int &Minute) {
	time_t t = static_cast<int32_t>(time(nullptr));
	struct tm *Time = localtime(&t);

	Hour = (Time->tm_min * 60 + Time->tm_sec) / SECONDS_PER_GAME_HOUR;
	Minute = (Time->tm_min * 60 + Time->tm_sec) % SECONDS_PER_GAME_HOUR * 2 / 5;
}

void GetRealTime(int &Hour, int &Minute) {
	time_t t = static_cast<int32_t>(time(nullptr));
	struct tm *Time = localtime(&t);

	Hour = Time->tm_hour;
	Minute = Time->tm_min;
}

uint32_t GetRoundAtTime(int Hour, int Minute) {
	time_t t = static_cast<int32_t>(time(nullptr));
	struct tm *Time = localtime(&t);

	int Delta = (Hour - Time->tm_hour) * 3600 + (Minute - Time->tm_min) * 60 - Time->tm_sec;
	if (Delta < 0) {
		Delta += SECONDS_PER_DAY;

	}
	return RoundNr + Delta;
}

uint32_t GetRoundForNextMinute() {
	int Hour, Minute;

	GetRealTime(Hour, Minute);
	uint32_t NewRound = GetRoundAtTime(Hour, Minute + 1) + 30;
	while (NewRound < RoundNr + 30) {
		NewRound += 60;
	}
	while (NewRound > RoundNr + 90) {
		NewRound -= 60;
	}
	return NewRound;
}

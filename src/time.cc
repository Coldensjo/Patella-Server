#include <cstdint>
#include <ctime>

#include "time.hh"

uint32_t RoundNr = 0;
uint32_t ServerMilliseconds = 0;

// time_t was a 32-bit long in the original; time() is narrowed to that width
// wherever it is stored.

void GetAmbiente(int &Brightness, int &Color) {
	int Hour, Minute;

	GetTime(Hour, Minute);
	int Time = Hour * 60 + Minute;
	if (Time < 60) {
		Brightness = 51;
		Color = 215;
	} else if (Time < 120) {
		Brightness = 102;
		Color = 215;
	} else if (Time < 180) {
		Brightness = 153;
		Color = 173;
	} else if (Time < 240) {
		Brightness = 204;
		Color = 173;
	} else if (Time > 1380) {
		Brightness = 51;
		Color = 215;
	} else if (Time > 1320) {
		Brightness = 102;
		Color = 215;
	} else if (Time > 1260) {
		Brightness = 153;
		Color = 208;
	} else if (Time > 1200) {
		Brightness = 204;
		Color = 208;
	} else {
		Brightness = 255;
		Color = 215;
	}
}

void GetDate(int &Year, int &Cycle, int &Day) {
	time_t t = static_cast<int32_t>(time(nullptr));
	struct tm *Time = localtime(&t);

	Year = (static_cast<int32_t>(t) / 86400 + 4) / 7;
	Cycle = Time->tm_wday;
	Day = Time->tm_hour;
}

void GetTime(int &Hour, int &Minute) {
	time_t t = static_cast<int32_t>(time(nullptr));
	struct tm *Time = localtime(&t);

	Hour = (Time->tm_min * 60 + Time->tm_sec) / 150;
	Minute = (Time->tm_min * 60 + Time->tm_sec) % 150 * 2 / 5;
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
		Delta += 86400;
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

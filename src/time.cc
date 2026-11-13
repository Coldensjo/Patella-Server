#include <cstdint>
#include <ctime>

#include "time.hh"

uint32_t RoundNr = 0;	// time.c:12
uint32_t ServerMilliseconds = 0;	// time.c:13

// time_t was a 32-bit long in the original; time() is narrowed to that width
// wherever it is stored.

// 0x08053c00  time.c:19
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

// 0x08053d10  time.c:43
void GetDate(int &Year, int &Cycle, int &Day) {
	time_t t = static_cast<int32_t>(time(nullptr));
	struct tm *Time = localtime(&t);

	Year = (static_cast<int32_t>(t) / 86400 + 4) / 7;
	Cycle = Time->tm_wday;
	Day = Time->tm_hour;
}

// 0x08053d90  time.c:56
void GetTime(int &Hour, int &Minute) {
	time_t t = static_cast<int32_t>(time(nullptr));
	struct tm *Time = localtime(&t);

	Hour = (Time->tm_min * 60 + Time->tm_sec) / 150;
	Minute = (Time->tm_min * 60 + Time->tm_sec) % 150 * 2 / 5;
}

// 0x08053e20  time.c:67
void GetRealTime(int &Hour, int &Minute) {
	time_t t = static_cast<int32_t>(time(nullptr));
	struct tm *Time = localtime(&t);

	Hour = Time->tm_hour;
	Minute = Time->tm_min;
}

// 0x08053e60  time.c:78
uint32_t GetRoundAtTime(int Hour, int Minute) {
	time_t t = static_cast<int32_t>(time(nullptr));
	struct tm *Time = localtime(&t);

	int Delta = (Hour - Time->tm_hour) * 3600 + (Minute - Time->tm_min) * 60 - Time->tm_sec;
	if (Delta < 0) {
		Delta += 86400;
	}
	return RoundNr + Delta;
}

// 0x08053ed0  time.c:90
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

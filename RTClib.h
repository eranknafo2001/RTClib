#ifndef __TIME_MANEGER_H__
#define __TIME_MANEGER_H__

#include "stdint.h"

class String;

class TimeSpan
{
public:
	TimeSpan(int32_t seconds = 0);
	TimeSpan(int16_t days, int8_t hours, int8_t minutes, int8_t seconds);
	TimeSpan(const TimeSpan& copy);
	int16_t days() const { return _seconds / 86400L; }
	int8_t  hours() const { return _seconds / 3600 % 24; }
	int8_t  minutes() const { return _seconds / 60 % 60; }
	int8_t  seconds() const { return _seconds % 60; }
	int32_t totalseconds() const { return _seconds; }

	TimeSpan operator+(const TimeSpan& right);
	TimeSpan operator-(const TimeSpan& right);

private:
	int32_t _seconds;
};

class DateTime
{
public:
	DateTime(uint32_t t = 0);
	DateTime(uint16_t year, uint8_t month, uint8_t day,
		uint8_t hour = 0, uint8_t min = 0, uint8_t sec = 0);

	DateTime(const DateTime& copy);
	DateTime(const char* date, const char* time);

	uint16_t year() const { return 2000 + yOff; }
	uint8_t month() const { return m; }
	uint8_t day() const { return dataPins; }
	uint8_t hour() const { return hh; }
	uint8_t minute() const { return mm; }
	uint8_t second() const { return ss; }
	uint8_t dayOfTheWeek() const;

	// 32-bit times as seconds since 1/1/2000
	long secondstime() const;
	// 32-bit times as seconds since 1/1/1970
	uint32_t unixtime() const;

	void toString(String& out);
	String toString();

	void toDateString(String& out);
	String toDateString();

	void toTimeString(String& out);
	String toTimeString();

	DateTime operator+(const TimeSpan& span);
	DateTime operator-(const TimeSpan& span);
	TimeSpan operator-(const DateTime& right);

private:
	uint8_t yOff, m, dataPins, hh, mm, ss;
};

class DS1307
{
public:
	DS1307();
	bool init();

	bool adjust(const DateTime& dt);

	bool adjust(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec);

	bool now(uint16_t& year, uint8_t& month, uint8_t& day, uint8_t& hour, uint8_t& min, uint8_t& sec);

	bool now(uint16_t* year = nullptr, uint8_t* month = nullptr, uint8_t* day = nullptr, uint8_t* hour = nullptr, uint8_t* min = nullptr, uint8_t* sec = nullptr);

	bool now(DateTime& out);
	bool now(String& out);
	bool nowTime(String & out);

	bool isRunning();

	bool getIsInitialized();
private:
	bool rNow(DateTime& out);

	bool rNow(uint16_t& year, uint8_t& month, uint8_t& day, uint8_t& hour, uint8_t& min, uint8_t& sec);

	bool rNow(uint16_t* year = nullptr, uint8_t* month = nullptr, uint8_t* day = nullptr, uint8_t* hour = nullptr, uint8_t* min = nullptr, uint8_t* sec = nullptr);


	bool isInitialized;
	DateTime ProgramStartTime;
};

#endif


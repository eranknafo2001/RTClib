// 
// 
// 

#include "RTClib.h"
#include <Wire.h>
#include <WString.h>

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#define _I2C_WRITE write
#define _I2C_READ  read
#else
#include "WProgram.h"
#define _I2C_WRITE send
#define _I2C_READ  receive
#endif

#define DS1307_ADDRESS 0x68

#define SECONDS_PER_DAY 86400L
#define SECONDS_FROM_1970_TO_2000 946684800

#define daysInMonth(m) (((m) == 2) ? 28 : (m)%2 == 0 ? 30 : 31)
#define DaysToHours(days) ((days) * 24)
#define HoursToMin(hours) ((hours) * 60)
#define MinToSec(sec) ((sec) * 60)

#pragma region DateTime Implementation

// number of days since 2000/01/01, valid for 2001..2099
static uint16_t date2days(uint16_t y, uint8_t m, uint8_t dataPins)
{
	if (y >= 2000)
		y -= 2000;
	uint16_t days = dataPins;
	for (uint8_t value = 1; value < m; ++value)
		days += daysInMonth(value);
	if (m > 2 && y % 4 == 0)
		++days;
	return days + 365 * y + (y + 3) / 4 - 1;
}

static long time2long(uint16_t days, uint8_t h, uint8_t m, uint8_t s)
{
	return ((days * 24L + h) * 60 + m) * 60 + s;
}

DateTime::DateTime(uint32_t t)
{
	t -= SECONDS_FROM_1970_TO_2000;    // bring to 2000 timestamp from 1970

	ss = t % 60;
	t /= 60;
	mm = t % 60;
	t /= 60;
	hh = t % 24;
	uint16_t days = t / 24;
	uint8_t leap;
	for (yOff = 0; ; ++yOff)
	{
		leap = yOff % 4 == 0;
		if (days < 365 + leap)
			break;
		days -= 365 + leap;
	}
	for (m = 1; ; ++m)
	{
		uint8_t daysPerMonth = daysInMonth(m);
		if (leap && m == 2)
			++daysPerMonth;
		if (days < daysPerMonth)
			break;
		days -= daysPerMonth;
	}
	dataPins = days + 1;
}

DateTime::DateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec)
{
	if (year >= 2000)
		year -= 2000;
	yOff = year;
	m = month;
	dataPins = day;
	hh = hour;
	mm = min;
	ss = sec;
}

DateTime::DateTime(const DateTime& copy) :
	yOff(copy.yOff),
	m(copy.m),
	dataPins(copy.dataPins),
	hh(copy.hh),
	mm(copy.mm),
	ss(copy.ss)
{
}

static uint8_t conv2d(const char* p)
{
	return (('0' <= p[0] && p[0] <= '9') ? (10 * (p[0] - '0')) : 0) + p[1] - '0';
}

// A convenient constructor for using "the compiler's time":
//   DateTime now (__DATE__, __TIME__);
// NOTE: using F() would further reduce the RAM footprint, see below.
DateTime::DateTime(const char* date, const char* time)
{
	// sample input: date = "Dec 26 2009", time = "12:34:56"
	yOff = conv2d(date + 9);
	// Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec 
	switch (date[0])
	{
	case 'J': m = (date[1] == 'a') ? 1 : ((date[2] == 'n') ? 6 : 7); break;
	case 'F': m = 2; break;
	case 'A': m = date[2] == 'r' ? 4 : 8; break;
	case 'M': m = date[2] == 'r' ? 3 : 5; break;
	case 'S': m = 9; break;
	case 'O': m = 10; break;
	case 'N': m = 11; break;
	case 'D': m = 12; break;
	}
	dataPins = conv2d(date + 4);
	hh = conv2d(time);
	mm = conv2d(time + 3);
	ss = conv2d(time + 6);
}

uint8_t DateTime::dayOfTheWeek() const
{
	uint16_t day = date2days(yOff, m, dataPins);
	return (day + 6) % 7; // Jan 1, 2000 is a Saturday, i.e. returns 6
}

uint32_t DateTime::unixtime(void) const
{
	uint32_t t;
	uint16_t days = date2days(yOff, m, dataPins);
	t = time2long(days, hh, mm, ss);
	t += SECONDS_FROM_1970_TO_2000;  // seconds from 1970 to 2000

	return t;
}

void DateTime::toString(String& out)
{
	String date;
	String time;
	toDateString(date);
	toTimeString(time);
	out = time + " " + date;
}

String DateTime::toString()
{
	String str;
	toString(str);
	return str;
}

void DateTime::toDateString(String& out)
{
	out = String(dataPins) + "/"
		+ String(m) + "/"
		+ String(year());
}

String DateTime::toDateString()
{
	String str;
	toDateString(str);
	return str;
}

void DateTime::toTimeString(String& out)
{
	String h(hh);
	String m(mm);
	String s(ss);
	if (s.length() == 1)
		s = "0" + s;
	if (m.length() == 1)
		m = "0" + m;
	if (h.length() == 1)
		h = "0" + h;
	out = h + ":"
		+ m + ":"
		+ s;
}

String DateTime::toTimeString()
{
	String str;
	toTimeString(str);
	return str;
}

long DateTime::secondstime(void) const
{
	return time2long(date2days(yOff, m, dataPins), hh, mm, ss);
}

DateTime DateTime::operator+(const TimeSpan& span)
{
	return DateTime(unixtime() + span.totalseconds());
}

DateTime DateTime::operator-(const TimeSpan& span)
{
	return DateTime(unixtime() - span.totalseconds());
}

TimeSpan DateTime::operator-(const DateTime& right)
{
	return TimeSpan(unixtime() - right.unixtime());
}

#pragma endregion

#pragma region TimeSpan Implementation

TimeSpan::TimeSpan(int32_t seconds) :
	_seconds(seconds)
{
}

TimeSpan::TimeSpan(int16_t days, int8_t hours, int8_t minutes, int8_t seconds) :
	_seconds(MinToSec(HoursToMin(DaysToHours(days) + hours) + minutes) + seconds)
{
}

TimeSpan::TimeSpan(const TimeSpan& copy) :
	_seconds(copy._seconds)
{
}

TimeSpan TimeSpan::operator+(const TimeSpan& right)
{
	return TimeSpan(_seconds + right._seconds);
}

TimeSpan TimeSpan::operator-(const TimeSpan& right)
{
	return TimeSpan(_seconds - right._seconds);
}

#pragma endregion

#pragma region TimeManeger Implementation

DS1307::DS1307() : isInitialized(false)
{
}

bool DS1307::init()
{
	Wire.begin();
	if (isRunning())
	{
		DateTime n;
		if (rNow(n))
			ProgramStartTime = n - TimeSpan(millis() / 1000);
	}
	isInitialized = true;
	return true;
}

uint8_t bcd2bin(uint8_t val) { return val - 6 * (val >> 4); }
uint8_t bin2bcd(uint8_t val) { return val + 6 * (val / 10); }

bool DS1307::adjust(const DateTime& dt)
{
	return adjust(dt.year(), dt.month(), dt.day(), dt.hour(), dt.minute(), dt.second());
}

bool DS1307::adjust(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec)
{
	Wire.beginTransmission(DS1307_ADDRESS);
	Wire._I2C_WRITE((byte)0); // start at location 0
	Wire._I2C_WRITE(bin2bcd(sec));
	Wire._I2C_WRITE(bin2bcd(min));
	Wire._I2C_WRITE(bin2bcd(hour));
	Wire._I2C_WRITE(bin2bcd(0));
	Wire._I2C_WRITE(bin2bcd(day));
	Wire._I2C_WRITE(bin2bcd(month));
	Wire._I2C_WRITE(bin2bcd(year - 2000));
	if (Wire.endTransmission() != 0)
		return false;
	if (isRunning())
	{
		DateTime n;
		if (rNow(n))
			ProgramStartTime = n - TimeSpan(millis() / 1000);
	}
	return true;
}

bool DS1307::rNow(DateTime& out)
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	if (!rNow(year, month, day, hour, min, sec)) return false;
	out = DateTime(year, month, day, hour, min, sec);
	return true;
}

bool DS1307::rNow(uint16_t& year, uint8_t& month, uint8_t& day, uint8_t& hour, uint8_t& min, uint8_t& sec)
{
	return rNow(&year, &month, &day, &hour, &min, &sec);
}

bool DS1307::rNow(uint16_t* year = nullptr, uint8_t* month = nullptr, uint8_t* day = nullptr, uint8_t* hour = nullptr, uint8_t* min = nullptr, uint8_t* sec = nullptr)
{
	Wire.beginTransmission(DS1307_ADDRESS);
	Wire._I2C_WRITE((byte)0);
	if (Wire.endTransmission() != 0)
		return false;
	if (Wire.requestFrom(DS1307_ADDRESS, 7) != 7)
		return false;
	uint8_t ss = bcd2bin(Wire._I2C_READ() & 0x7F);
	uint8_t mm = bcd2bin(Wire._I2C_READ());
	uint8_t hh = bcd2bin(Wire._I2C_READ());
	Wire._I2C_READ();
	uint8_t d = bcd2bin(Wire._I2C_READ());
	uint8_t m = bcd2bin(Wire._I2C_READ());
	uint16_t y = bcd2bin(Wire._I2C_READ()) + 2000;
	if (year != nullptr)
		* year = y;
	if (month != nullptr)
		* month = m;
	if (day != nullptr)
		* day = d;
	if (hour != nullptr)
		* hour = hh;
	if (min != nullptr)
		* min = mm;
	if (sec != nullptr)
		* sec = ss;

	return true;
}

bool DS1307::now(uint16_t& year, uint8_t& month, uint8_t& day, uint8_t& hour, uint8_t& min, uint8_t& sec)
{
	return now(&year, &month, &day, &hour, &min, &sec);
}

bool DS1307::now(uint16_t* year = nullptr, uint8_t* month = nullptr, uint8_t* day = nullptr, uint8_t* hour = nullptr, uint8_t* min = nullptr, uint8_t* sec = nullptr)
{
	DateTime dt;
	if (!now(dt)) return false;
	if (year != nullptr)
		* year = dt.year();
	if (month != nullptr)
		* month = dt.month();
	if (day != nullptr)
		* day = dt.day();
	if (hour != nullptr)
		* hour = dt.hour();
	if (min != nullptr)
		* min = dt.minute();
	if (sec != nullptr)
		* sec = dt.second();
	return true;
}

bool DS1307::now(DateTime& out)
{
	if (!getIsInitialized())
		return false;
	if (!isRunning())
		return false;
	out = ProgramStartTime + TimeSpan(millis() / 1000);
	return true;
}

bool DS1307::now(String& out)
{
	DateTime date;
	if (!now(date))
		return false;
	date.toString(out);
	return true;
}

bool DS1307::nowTime(String& out)
{
	DateTime date;
	if (!now(date))
		return false;
	date.toTimeString(out);
	return true;
}

bool DS1307::isRunning()
{
	Wire.beginTransmission(DS1307_ADDRESS);
	Wire._I2C_WRITE((byte)0);
	if (Wire.endTransmission() != 0)
		return false;

	if (Wire.requestFrom(DS1307_ADDRESS, 1) != 1)
		return false;
	uint8_t ss = Wire._I2C_READ();
	return !(ss >> 7);
}

bool DS1307::getIsInitialized()
{
	return isInitialized;
}

#pragma endregion


/*
  DS3234 library for the Arduino.

  This library implements the following features:

   - read/write of current time, both of the alarms, 
   control/status registers, aging register, sram
   - read of the temperature register, and of any address from the chip.

  Author:          Petre Rodan <petre.rodan@simplex.ro>
  Available from:  https://github.com/rodan/ds3234
 
  The DS3231 is a low-cost, extremely accurate I2C real-time clock 
  (RTC) with an integrated temperature-compensated crystal oscillator 
  (TCXO) and crystal.

  GNU GPLv3 license:
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
   
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
   
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
   
*/

#include <SPI.h>
#include <ds3234.h>

// helpers

uint8_t dectobcd(const uint8_t val)
{
    return ((val / 10 * 16) + (val % 10));
}

uint8_t bcdtodec(const uint8_t val)
{
    return ((val / 16 * 10) + (val % 16));
}

// class

DS3234RTC::DS3234RTC( uint8_t pin )
{
	ss_pin = pin;
	pinMode(ss_pin, OUTPUT);
	spi_settings = SPISettings(4000000, MSBFIRST, SPI_MODE1);
}

DS3234RTC::DS3234RTC( uint8_t pin, const uint8_t ctrl_reg )
{
	ss_pin = pin;
	pinMode(ss_pin, OUTPUT);
	spi_settings = SPISettings(4000000, MSBFIRST, SPI_MODE1);
}

bool DS3234RTC::available() {
	return 1;
}

time_t DS3234RTC::get()
{
	tmElements_t tm;
	read(tm);
	return makeTime(tm);
}

uint8_t DS3234RTC::read1(uint8_t addr)
{
	uint8_t data;

	SPI.beginTransaction(spi_settings);
	digitalWrite(ss_pin, LOW);
	SPI.transfer(addr);
	data = SPI.transfer(0x00);
	digitalWrite(ss_pin, HIGH);
	SPI.endTransaction();
	return data;
}

void DS3234RTC::write1(uint8_t addr, uint8_t value)
{
	SPI.beginTransaction(spi_settings);
	digitalWrite(ss_pin, LOW);
	SPI.transfer(addr + 0x80);
	SPI.transfer(value);
	digitalWrite(ss_pin, HIGH);
	SPI.endTransaction();
}

void DS3234RTC::read( tmElements_t &tm )
{
	uint8_t TimeDate[7];      //second,minute,hour,dow,day,month,year
	uint8_t century = 0;
	uint8_t i;

	SPI.beginTransaction(spi_settings);
	delay(1);
	digitalWrite(ss_pin, LOW);
	SPI.transfer(0x00);       // Request transfer of date/time registers
	for (i = 0; i <= 6; i++)
	{
	  TimeDate[i] = SPI.transfer(0x00);
	}
	digitalWrite(ss_pin, HIGH);
	SPI.endTransaction();

	tm.Second = bcdtodec(TimeDate[0] & 0x7F);
	tm.Minute = bcdtodec(TimeDate[1] & 0x7F);
	if ((TimeDate[2] & 0x40) != 0)
	{
		// Convert 12-hour format to 24-hour format
		tm.Hour = bcdtodec(TimeDate[2] & 0x1F);
		if((TimeDate[2] & 0x20) != 0) tm.Hour += 12;
	} else {
		tm.Hour = bcdtodec(TimeDate[2] & 0x3F);
	}
	tm.Wday = bcdtodec(TimeDate[3] & 0x07);
	tm.Day = bcdtodec(TimeDate[4] & 0x3F);
	tm.Month = bcdtodec(TimeDate[5] & 0x1F);
	tm.Year = bcdtodec(TimeDate[6]);
	century = (TimeDate[5] & 0x80);
	if (century != 0) tm.Year += 100;
	tm.Year = y2kYearToTm(tm.Year);
}

void DS3234RTC::writeDate( tmElements_t &tm )
{
	uint8_t i, y;
	uint8_t TimeDate[7];

	if( tm.Wday == 0 || tm.Wday > 7)
	{
		tmElements_t tm2;
		breakTime( makeTime(tm), tm2 );  // Calculate Wday by converting to Unix time and back
		tm.Wday = tm2.Wday;
	}
	TimeDate[3] = tm.Wday;
	TimeDate[4] = dectobcd(tm.Day);
	TimeDate[5] = dectobcd(tm.Month);
	y = tmYearToY2k(tm.Year);
	if (y > 99)
	{
		TimeDate[5] |= 0x80; // century flag
		y -= 100;
	}
	TimeDate[6] = dectobcd(y);

	// Write date to RTC
	SPI.beginTransaction(spi_settings);
	digitalWrite(ss_pin, LOW);
	SPI.transfer(0x83);           // Request write into date registers
	for (i = 3; i <= 6; i++)      // For sanity, index is register we're writing
	{
		SPI.transfer(TimeDate[i]);
	}
	digitalWrite(ss_pin, HIGH);
	SPI.endTransaction();
}

void DS3234RTC::writeTime( tmElements_t &tm )
{
	uint8_t i;
	uint8_t TimeDate[7];

	TimeDate[0] = dectobcd(tm.Second);
	TimeDate[1] = dectobcd(tm.Minute);
	TimeDate[2] = dectobcd(tm.Hour);

	SPI.beginTransaction(spi_settings);
	digitalWrite(ss_pin, LOW);
	SPI.transfer(0x80);           // Request write into time registers
	for (i = 0; i < 3; i++)
	{
		SPI.transfer(TimeDate[i]);
	}
	digitalWrite(ss_pin, HIGH);
	SPI.endTransaction();
}

void DS3234RTC::readTemperature(tpElements_t &tmp)
{
	uint8_t msb, lsb;

	SPI.beginTransaction(spi_settings);
	digitalWrite(ss_pin, LOW);
	SPI.transfer(0x11);
	msb = SPI.transfer(0x00);
	lsb = SPI.transfer(0x00);
	digitalWrite(ss_pin, HIGH);
	SPI.endTransaction();

	tmp.Temp = msb;
	tmp.Decimal = (lsb >> 6) * 25;
}

void DS3234RTC::readAlarm(uint8_t alarm, alarmMode_t &mode, tmElements_t &tm)
{
	uint8_t data[4];
	uint8_t flags;

	memset(&tm, 0, sizeof(tmElements_t));
	mode = alarmModeUnknown;
	if ((alarm < 1) || (alarm > 2)) return;

	SPI.beginTransaction(spi_settings);
	digitalWrite(ss_pin, LOW);
	SPI.transfer( (alarm==1) ? 0x07 : 0x08);
	data[0] = 0;
	if (alarm == 1) data[0] = SPI.transfer(0x00);
	data[1] = SPI.transfer(0x00);
	data[2] = SPI.transfer(0x00);
	data[3] = SPI.transfer(0x00);
	digitalWrite(ss_pin, HIGH);
	SPI.endTransaction();

	flags =
		((data[0] & 0x80) >> 7) |
		((data[1] & 0x80) >> 6) |
		((data[2] & 0x80) >> 5) |
		((data[3] & 0x80) >> 4);
	if (flags == 0) flags = ((data[3] & 0x40) >> 2);
	switch (flags) {
		case 0x04: mode = alarmModePerSecond; break;  // X1111
		case 0x0E: mode = (alarm == 1) ? alarmModeSecondsMatch : alarmModePerMinute; break;  // X1110
		case 0x0A: mode = alarmModeMinutesMatch; break;  // X1100
		case 0x08: mode = alarmModeHoursMatch; break;  // X1000
		case 0x00: mode = alarmModeDateMatch; break;  // 00000
		case 0x10: mode = alarmModeDayMatch; break;  // 10000
	}

	if (alarm == 1) tm.Second = bcdtodec(data[0] & 0x7F);
	tm.Minute = bcdtodec(data[1] & 0x7F);
	if ((data[2] & 0x40) != 0) {
		// 12 hour format with bit 5 set as PM
		tm.Hour = bcdtodec(data[2] & 0x1F);
		if ((data[2] & 0x20) != 0) tm.Hour += 12;
	} else {
		// 24 hour format
		tm.Hour = bcdtodec(data[2] & 0x3F);
	}
	if ((data[3] & 0x40) == 0) {
		// Alarm holds Date (of Month)
		tm.Day = bcdtodec(data[3] & 0x3F);
	} else {
		// Alarm holds Day (of Week)
		tm.Wday = bcdtodec(data[3] & 0x07);
	}

	// TODO : Not too sure about this.
	/*
	If the alarm is set to trigger every Nth of the month
	(or every 1-7 week day), but the date/day are 0 then
	what?  The spec is not clear about alarm off conditions.
	My assumption is that it would not trigger is date/day
	set to 0, so I've created a Alarm-Off mode.
	*/
	if ((mode == alarmModeDateMatch) && (tm.Day == 0)) {
		mode = alarmModeOff;
	} else if ((mode == alarmModeDayMatch) && (tm.Wday == 0)) {
		mode = alarmModeOff;
	}
}

void DS3234RTC::writeAlarm(uint8_t alarm, alarmMode_t mode, tmElements_t tm) {
	uint8_t data[4];

	switch (mode) {
		case alarmModePerSecond:
			data[0] = 0x80;
			data[1] = 0x80;
			data[2] = 0x80;
			data[3] = 0x80;
			break;
		case alarmModePerMinute:
			data[0] = 0x00;
			data[1] = 0x80;
			data[2] = 0x80;
			data[3] = 0x80;
			break;
		case alarmModeSecondsMatch:
			data[0] = 0x00 | dectobcd(tm.Second);
			data[1] = 0x80;
			data[2] = 0x80;
			data[3] = 0x80;
			break;
		case alarmModeMinutesMatch:
			data[0] = 0x00 | dectobcd(tm.Second);
			data[1] = 0x00 | dectobcd(tm.Minute);
			data[2] = 0x80;
			data[3] = 0x80;
			break;
		case alarmModeHoursMatch:
			data[0] = 0x00 | dectobcd(tm.Second);
			data[1] = 0x00 | dectobcd(tm.Minute);
			data[2] = 0x00 | dectobcd(tm.Hour);
			data[3] = 0x80;
			break;
		case alarmModeDateMatch:
			data[0] = 0x00 | dectobcd(tm.Second);
			data[1] = 0x00 | dectobcd(tm.Minute);
			data[2] = 0x00 | dectobcd(tm.Hour);
			data[3] = 0x00 | dectobcd(tm.Day);
			break;
		case alarmModeDayMatch:
			data[0] = 0x00 | dectobcd(tm.Second);
			data[1] = 0x00 | dectobcd(tm.Minute);
			data[2] = 0x00 | dectobcd(tm.Hour);
			data[3] = 0x40 | dectobcd(tm.Wday);
			break;
		case alarmModeOff:
			data[0] = 0x00;
			data[1] = 0x00;
			data[2] = 0x00;
			data[3] = 0x00;
			break;
		default: return;
	}

	SPI.beginTransaction(spi_settings);
	digitalWrite(ss_pin, LOW);
	SPI.transfer( ((alarm == 1) ? 0x07 : 0x0B) );
	if (alarm == 1) SPI.transfer(data[0]);
	SPI.transfer(data[1]);
	SPI.transfer(data[2]);
	SPI.transfer(data[3]);
	digitalWrite(ss_pin, HIGH);
	SPI.endTransaction();
}

bool DS3234RTC::isAlarmInterrupt(uint8_t alarm)
{
	if ((alarm > 2) || (alarm < 1)) return false;
	uint8_t value = readControlRegister() & 0x07;  // sends 0Eh - Control register
	if (alarm == 1) {
		return ((value & 0x05) == 0x05);
	} else {
		return ((value & 0x06) == 0x06);
	}
}

uint8_t DS3234RTC::isAlarmFlag()
{
	uint8_t value = readStatusRegister(); // Status register
	return (value & (DS3234_A1F | DS3234_A2F));
}

bool DS3234RTC::isAlarmFlag(uint8_t alarm)
{
	uint8_t value = isAlarmFlag();
	return ((value & alarm) != 0);
}

void DS3234RTC::clearAlarmFlag(uint8_t alarm)
{
	uint8_t alarm_mask, value;

	if ((alarm != 1) and (alarm != 2)) return;
	alarm_mask = ~alarm;

	value = readStatusRegister();
	value &= alarm_mask;
	writeStatusRegister(value);
}

/*
 * Control Register
 *
 *   ~EOSC  BBSQW  CONV  RS2  RS1  INTCN  A2IE  A1IE
 */

uint8_t DS3234RTC::readControlRegister()
{
	return read1(0x0E);
}

void DS3234RTC::writeControlRegister(uint8_t value)
{
	write1(0x0E, value);
}

void DS3234RTC::setBBOscillator(bool enable)
{
	uint8_t value = readControlRegister();
	if (enable)
	{
		value |= DS3232_EOSC;
	} else {
		value &= ~DS3232_EOSC;
	}
	writeControlRegister(value);
}

void DS3234RTC::setBBSqareWave(bool enable)
{
	uint8_t value = readControlRegister();
	if (enable)
	{
		value |= DS3232_BBSQW;
	} else {
		value &= ~DS3232_BBSQW;
	}
	writeControlRegister(value);
}

void DS3234RTC::setSQIMode(sqiMode_t mode)
{
	uint8_t value = readControlRegister();
	switch (mode)
	{
		case sqiModeNone: value |= DS3232_INTCN; break;
		case sqiMode1Hz: value |= DS3232_RS_1HZ;  break;
		case sqiMode1024Hz: value |= DS3232_RS_1024HZ; break;
		case sqiMode4096Hz: value |= DS3232_RS_4096HZ; break;
		case sqiMode8192Hz: value |= DS3232_RS_8192HZ; break;
		case sqiModeAlarm1: value |= (DS3232_INTCN | DS3232_A1IE); break;
		case sqiModeAlarm2: value |= (DS3232_INTCN | DS3232_A2IE); break;
		case sqiModeAlarmBoth: value |= (DS3232_INTCN | DS3232_A1IE | DS3232_A2IE); break;
	}
	writeControlRegister(value);
}

/*
 * Status Register
 *
 *   OSF  BB33KHZ  CRATE1  CRATE0  EN33KHZ  BSY  A2F  A1F
 */
uint8_t DS3234RTC::readStatusRegister()
{
	return read1(0x0F);
}

void DS3234RTC::writeStatusRegister(uint8_t value)
{
	write1(0x0F, value);
}

bool DS3234RTC::isOscillatorStopFlag()
{
	return ((readStatusRegister() & DS3234_OSF) != 0);
}

void DS3234RTC::setOscillatorStopFlag(bool enable)
{
	uint8_t value = readStatusRegister();
	if (enable)
	{
		value |= DS3234_OSF;
	} else {
		value &= ~DS3234_OSF;
	}
	writeStatusRegister(value);
}

void DS3234RTC::setBB33kHzOutput(bool enable)
{
	uint8_t value = readStatusRegister();
	if (enable)
	{
		value |= DS3234_BB33KHZ;
	} else {
		value &= ~DS3234_BB33KHZ;
	}
}

void DS3234RTC::setTCXORate(tempScanRate_t rate)
{
	uint8_t value = readStatusRegister() & 0xCF; // clear the rate bits
	switch (rate)
	{
		case tempScanRate64sec: value |= DS3232_CRATE_64; break;
		case tempScanRate128sec: value |= DS3232_CRATE_128; break;
		case tempScanRate256sec: value |= DS3232_CRATE_256; break;
		case tempScanRate512sec: value |= DS3232_CRATE_512; break;
	}
	writeStatusRegister(value);
}

void DS3234RTC::set33kHzOutput(bool enable)
{
	uint8_t value = readStatusRegister();
	if (enable)
	{
		value |= DS3232_EN33KHZ;
	} else {
		value &= ~DS3232_EN33KHZ;
	}
	writeStatusRegister(value);
}

bool DS3234RTC::isTCXOBusy()
{
	return((readStatusRegister() & DS3232_BSY) != 0);
}

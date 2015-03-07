
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
#include <stdio.h>
#include <ds3234.h>

void DS3234_init(const uint8_t pin, const uint8_t ctrl_reg)
{
    pinMode(pin, OUTPUT);       // chip select pin
    SPI.begin();
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE1);
    DS3234_set_creg(pin, ctrl_reg);
    delay(10);
}

void DS3234_set(const uint8_t pin, struct ts t)
{
    uint8_t i, century;

    if (t.year > 2000) {
        century = 0x80;
        t.year_s = t.year - 2000;
    } else {
        century = 0;
        t.year_s = t.year - 1900;
    }

    uint8_t TimeDate[7] = { t.sec, t.min, t.hour, t.wday, t.mday, t.mon, t.year_s };
    for (i = 0; i <= 6; i++) {
        digitalWrite(pin, LOW);
        SPI.transfer(i + 0x80);
        if (i == 5)
            SPI.transfer(dectobcd(TimeDate[5]) + century);
        else
            SPI.transfer(dectobcd(TimeDate[i]));
        digitalWrite(pin, HIGH);
    }
}

void DS3234_get(const uint8_t pin, struct ts *t)
{
    uint8_t TimeDate[7];        //second,minute,hour,dow,day,month,year
    uint8_t century = 0;
    uint8_t i, n;
    uint16_t year_full;

    for (i = 0; i <= 6; i++) {
        digitalWrite(pin, LOW);
        SPI.transfer(i + 0x00);
        n = SPI.transfer(0x00);
        digitalWrite(pin, HIGH);
        if (i == 5) {           // month address also contains the century on bit7
            TimeDate[5] = bcdtodec(n & 0x1F);
            century = (n & 0x80) >> 7;
        } else {
            TimeDate[i] = bcdtodec(n);
        }
    }

    if (century == 1)
        year_full = 2000 + TimeDate[6];
    else
        year_full = 1900 + TimeDate[6];

    t->sec = TimeDate[0];
    t->min = TimeDate[1];
    t->hour = TimeDate[2];
    t->mday = TimeDate[4];
    t->mon = TimeDate[5];
    t->year = year_full;
    t->wday = TimeDate[3];
    t->year_s = TimeDate[6];
}

void DS3234_set_addr(const uint8_t pin, const uint8_t addr, const uint8_t val)
{
    digitalWrite(pin, LOW);
    SPI.transfer(addr);
    SPI.transfer(val);
    digitalWrite(pin, HIGH);
}

uint8_t DS3234_get_addr(const uint8_t pin, const uint8_t addr)
{
    uint8_t rv;

    digitalWrite(pin, LOW);
    SPI.transfer(addr);
    rv = SPI.transfer(0x00);
    digitalWrite(pin, HIGH);
    return rv;
}

/* control register 0Eh/8Eh
bit7 EOSC   Enable Oscillator (1 if oscillator must be stopped when on battery)
bit6 BBSQW  Battery Backed Square Wave
bit5 CONV   Convert temperature (1 forces a conversion NOW)
bit4 RS2    Rate select - frequency of square wave output
bit3 RS1    Rate select
bit2 INTCN  Interrupt control (1 for use of the alarms and to disable square wave)
bit1 A2IE   Alarm2 interrupt enable (1 to enable)
bit0 A1IE   Alarm1 interrupt enable (1 to enable)
*/

void DS3234_set_creg(const uint8_t pin, const uint8_t val)
{
    DS3234_set_addr(pin, 0x8E, val);
}

/* status register 0Fh/8Fh
bit7 OSF      Oscillator Stop Flag (if 1 then oscillator has stopped and date might be innacurate)
bit6 BB32kHz  Battery Backed 32kHz output (1 if square wave is needed when powered by battery)
bit5 CRATE1   Conversion rate 1  temperature compensation rate
bit4 CRATE0   Conversion rate 0  temperature compensation rate
bit3 EN32kHz  Enable 32kHz output (1 if needed)
bit2 BSY      Busy with TCXO functions
bit1 A2F      Alarm 2 Flag - (1 if alarm2 was triggered)
bit0 A1F      Alarm 1 Flag - (1 if alarm1 was triggered)
*/

void DS3234_set_sreg(const uint8_t pin, const uint8_t sreg)
{
    DS3234_set_addr(pin, 0x8F, sreg);
}

uint8_t DS3234_get_sreg(const uint8_t pin)
{
    uint8_t rv;
    rv = DS3234_get_addr(pin, 0x0f);
    return rv;
}

// aging register

void DS3234_set_aging(const uint8_t pin, const int8_t value)
{
    uint8_t reg;

    if (value >= 0)
        reg = value;
    else
        reg = ~(-value) + 1;    // 2C

    DS3234_set_addr(pin, 0x90, reg);
}

int8_t DS3234_get_aging(const uint8_t pin)
{
    uint8_t reg;
    int8_t rv;

    reg = DS3234_get_addr(pin, 0x10);
    if ((reg & 0x80) != 0)
        rv = reg | ~((1 << 8) - 1);     // if negative get two's complement
    else
        rv = reg;

    return rv;
}

// temperature register

float DS3234_get_treg(const uint8_t pin)
{
    float rv;
    uint8_t temp_msb, temp_lsb;
    int8_t nint;

    temp_msb = DS3234_get_addr(pin, 0x11);
    temp_lsb = DS3234_get_addr(pin, 0x12) >> 6;
    if ((temp_msb & 0x80) != 0)
        nint = temp_msb | ~((1 << 8) - 1);      // if negative get two's complement
    else
        nint = temp_msb;

    rv = 0.25 * temp_lsb + nint;

    return rv;
}

// alarms

// flags are: A1M1 (seconds), A1M2 (minutes), A1M3 (hour), 
// A1M4 (day) 0 to enable, 1 to disable, DY/DT (dayofweek == 1/dayofmonth == 0)
void DS3234_set_a1(const uint8_t pin, const uint8_t s, const uint8_t mi, const uint8_t h,
        const uint8_t d, const uint8_t * flags)
{
    uint8_t t[4] = { s, mi, h, d };
    uint8_t i;

    for (i = 0; i <= 3; i++) {
        digitalWrite(pin, LOW);
        SPI.transfer(i + 0x87);
        if (i == 3) {
            SPI.transfer(dectobcd(t[3]) | (flags[3] << 7) | (flags[4] << 6));
        } else
            SPI.transfer(dectobcd(t[i]) | (flags[i] << 7));
        digitalWrite(pin, HIGH);
    }
}

void DS3234_get_a1(const uint8_t pin, char *buf, const uint8_t len)
{
    uint8_t n[4];
    uint8_t t[4];               //second,minute,hour,day
    uint8_t f[5];               // flags
    uint8_t i;

    for (i = 0; i <= 3; i++) {
        digitalWrite(pin, LOW);
        SPI.transfer(i + 0x07);
        n[i] = SPI.transfer(0x00);
        digitalWrite(pin, HIGH);
        f[i] = (n[i] & 0x80) >> 7;
        t[i] = bcdtodec(n[i] & 0x7F);
    }

    f[4] = (n[3] & 0x40) >> 6;
    t[3] = bcdtodec(n[3] & 0x3F);

    snprintf(buf, len,
             "s%02d m%02d h%02d d%02d fs%d m%d h%d d%d wm%d %d %d %d %d",
             t[0], t[1], t[2], t[3], f[0], f[1], f[2], f[3], f[4], n[0],
             n[1], n[2], n[3]);

}

// when the alarm flag is cleared the pulldown on INT is also released
void DS3234_clear_a1f(const uint8_t pin)
{
    uint8_t reg_val;

    reg_val = DS3234_get_sreg(pin) & ~DS3234_A1F;
    DS3234_set_sreg(pin, reg_val);
}

uint8_t DS3234_triggered_a1(const uint8_t pin)
{
    return  DS3234_get_sreg(pin) & DS3234_A1F;
}

// flags are: A2M2 (minutes), A2M3 (hour), A2M4 (day) 0 to enable, 1 to disable, DY/DT (dayofweek == 1/dayofmonth == 0) - 
void DS3234_set_a2(const uint8_t pin, const uint8_t mi, const uint8_t h, const uint8_t d,
                   const uint8_t * flags)
{
    uint8_t t[3] = { mi, h, d };
    uint8_t i;

    for (i = 0; i <= 2; i++) {
        digitalWrite(pin, LOW);
        SPI.transfer(i + 0x8B);
        if (i == 2) {
            SPI.transfer(dectobcd(t[2]) | (flags[2] << 7) | (flags[3] << 6));
        } else
            SPI.transfer(dectobcd(t[i]) | (flags[i] << 7));
        digitalWrite(pin, HIGH);
    }
}

void DS3234_get_a2(const uint8_t pin, char *buf, const uint8_t len)
{
    uint8_t n[3];
    uint8_t t[3];               //second,minute,hour,day
    uint8_t f[4];               // flags
    uint8_t i;

    for (i = 0; i <= 2; i++) {
        digitalWrite(pin, LOW);
        SPI.transfer(i + 0x0B);
        n[i] = SPI.transfer(0x00);
        digitalWrite(pin, HIGH);
        f[i] = (n[i] & 0x80) >> 7;
        t[i] = bcdtodec(n[i] & 0x7F);
    }

    f[3] = (n[2] & 0x40) >> 6;
    t[2] = bcdtodec(n[2] & 0x3F);

    snprintf(buf, len, "m%02d h%02d d%02d fm%d h%d d%d wm%d %d %d %d", t[0],
             t[1], t[2], f[0], f[1], f[2], f[3], n[0], n[1], n[2]);

}

// when the alarm flag is cleared the pulldown on INT is also released
void DS3234_clear_a2f(const uint8_t pin)
{
    uint8_t reg_val;

    reg_val = DS3234_get_sreg(pin) & ~DS3234_A2F;
    DS3234_set_sreg(pin, reg_val);
}

uint8_t DS3234_triggered_a2(const uint8_t pin)
{
    return  DS3234_get_sreg(pin) & DS3234_A2F;
}

// sram

void DS3234_set_sram_8b(const uint8_t pin, const uint8_t address, const uint8_t value)
{
    DS3234_set_addr(pin, 0x98, address);
    DS3234_set_addr(pin, 0x99, value);
}

uint8_t DS3234_get_sram_8b(const uint8_t pin, const uint8_t address)
{
    uint8_t rv;

    DS3234_set_addr(pin, 0x98, address);
    rv = DS3234_get_addr(pin, 0x19);
    return rv;
}

// helpers

uint8_t dectobcd(const uint8_t val)
{
    return ((val / 10 * 16) + (val % 10));
}

uint8_t bcdtodec(const uint8_t val)
{
    return ((val / 16 * 10) + (val % 16));
}

uint8_t inp2toi(const char *cmd, const uint16_t seek)
{
    uint8_t rv;
    rv = (cmd[seek] - 48) * 10 + cmd[seek + 1] - 48;
    return rv;
}

// class

DS3234RTC::DS3234RTC( uint8_t pin )
{
	ss_pin = pin;
	pinMode(ss_pin, OUTPUT);
	spi_settings = SPISettings(1000000, MSBFIRST, SPI_MODE1);
}

DS3234RTC::DS3234RTC( uint8_t pin, const uint8_t ctrl_reg )
{
	ss_pin = pin;
	pinMode(ss_pin, OUTPUT);
	spi_settings = SPISettings(1000000, MSBFIRST, SPI_MODE1);
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

bool DS3234RTC::isAlarmInterrupt(uint8_t alarm)
{
	if ((alarm > 2) || (alarm < 1)) return false;
	uint8_t value = read1(0x0E) & 0x07;  // sends 0Eh - Control register
	if (alarm == 1) {
		return ((value & 0x05) == 0x05);
	} else {
		return ((value & 0x06) == 0x06);
	}
}

uint8_t DS3234RTC::isAlarmFlag()
{
	uint8_t value = read1(0x0F); // Status register
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

	value = read1(0x0F);
	value &= alarm_mask;
	write1(0x0F, value);
}


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
#include "ds3234.h"

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
        century = B10000000;
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
            century = (n & B10000000) >> 7;
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

// control register

void DS3234_set_creg(const uint8_t pin, const uint8_t val)
{
    digitalWrite(pin, LOW);
    SPI.transfer(0x8E);         // control register address
    SPI.transfer(val);
    digitalWrite(pin, HIGH);
}

// status register 0Fh/8Fh

/*
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
    digitalWrite(pin, LOW);
    SPI.transfer(0x8F);         // status register write address
    SPI.transfer(sreg);
    digitalWrite(pin, HIGH);
}

uint8_t DS3234_get_sreg(const uint8_t pin)
{
    uint8_t rv;

    digitalWrite(pin, LOW);
    SPI.transfer(0x0F);         // status register read address
    rv = SPI.transfer(0x00);
    digitalWrite(pin, HIGH);

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

    digitalWrite(pin, LOW);
    SPI.transfer(0x90);         // aging offset write register
    SPI.transfer(reg);
    digitalWrite(pin, HIGH);
}

int8_t DS3234_get_aging(const uint8_t pin)
{
    uint8_t reg;
    int8_t rv;

    digitalWrite(pin, LOW);
    SPI.transfer(0x10);         // aging offset register
    reg = SPI.transfer(0x00);
    digitalWrite(pin, HIGH);

    if ((reg & B10000000) != 0)
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

    digitalWrite(pin, LOW);
    SPI.transfer(0x11);         // temperature register MSB address
    temp_msb = SPI.transfer(0x00);
    digitalWrite(pin, HIGH);
    digitalWrite(pin, LOW);
    SPI.transfer(0x12);         // temperature register MSB address
    temp_lsb = SPI.transfer(0x00) >> 6;
    digitalWrite(pin, HIGH);

    if ((temp_msb & B10000000) != 0)
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
        const uint8_t d, const boolean * flags)
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

void DS3234_get_a1(const uint8_t pin, char *buf, const size_t len)
{
    uint8_t n[4];
    uint8_t t[4];               //second,minute,hour,day
    boolean f[5];               // flags
    uint8_t i;

    for (i = 0; i <= 3; i++) {
        digitalWrite(pin, LOW);
        SPI.transfer(i + 0x07);
        n[i] = SPI.transfer(0x00);
        digitalWrite(pin, HIGH);
        f[i] = (n[i] & B10000000) >> 7;
        t[i] = bcdtodec(n[i] & 0x7F);
    }

    f[4] = (n[3] & B01000000) >> 6;
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
                   const boolean * flags)
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

void DS3234_get_a2(const uint8_t pin, char *buf, const size_t len)
{
    uint8_t n[3];
    uint8_t t[3];               //second,minute,hour,day
    boolean f[4];               // flags
    uint8_t i;

    for (i = 0; i <= 2; i++) {
        digitalWrite(pin, LOW);
        SPI.transfer(i + 0x0B);
        n[i] = SPI.transfer(0x00);
        digitalWrite(pin, HIGH);
        f[i] = (n[i] & B10000000) >> 7;
        t[i] = bcdtodec(n[i] & 0x7F);
    }

    f[3] = (n[2] & B01000000) >> 6;
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
    digitalWrite(pin, LOW);
    SPI.transfer(0x98);
    SPI.transfer(address);
    digitalWrite(pin, HIGH);
    digitalWrite(pin, LOW);
    SPI.transfer(0x99);
    SPI.transfer(value);
    digitalWrite(pin, HIGH);
}

uint8_t DS3234_get_sram_8b(const uint8_t pin, const uint8_t address)
{
    uint8_t rv;

    digitalWrite(pin, LOW);
    SPI.transfer(0x98);
    SPI.transfer(address);
    digitalWrite(pin, HIGH);
    digitalWrite(pin, LOW);
    SPI.transfer(0x19);
    rv = SPI.transfer(0x00);
    digitalWrite(pin, HIGH);

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

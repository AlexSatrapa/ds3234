#ifndef __ds3234_h_
#define __ds3234_h_

#if ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include <Time.h>
#include <RTC.h>

// control register bits
#define DS3234_A1IE     0x01
#define DS3234_A2IE     0x02
#define DS3234_INTCN    0x04
#define DS3234_RS1      0x08
#define DS3234_RS2      0x10
#define DS3234_CONV     0x20
#define DS3234_BBSQ     0x40
#define DS3234_EOSC     0x80

// status register bits
#define DS3234_A1F      0x1
#define DS3234_A2F      0x2
#define DS3234_OSF      0x80

struct ts {
    uint8_t sec;         /* seconds */
    uint8_t min;         /* minutes */
    uint8_t hour;        /* hours */
    uint8_t mday;        /* day of the month */
    uint8_t mon;         /* month */
    int year;            /* year */
    uint8_t wday;        /* day of the week */
    uint8_t yday;        /* day in the year */
    uint8_t isdst;       /* daylight saving time */
    uint8_t year_s;      /* year in short notation*/
};

void DS3234_init(const uint8_t pin, const uint8_t creg);
void DS3234_set(const uint8_t pin, struct ts t);
void DS3234_get(const uint8_t pin, struct ts *t);

void DS3234_set_addr(const uint8_t pin, const uint8_t addr, const uint8_t val);
uint8_t DS3234_get_addr(const uint8_t pin, const uint8_t addr);

// control/status register
void DS3234_set_creg(const uint8_t pin, const uint8_t val);
void DS3234_set_sreg(const uint8_t pin, const uint8_t mask);
uint8_t DS3234_get_sreg(const uint8_t pin);

// aging offset register
void DS3234_set_aging(const uint8_t pin, const int8_t value);
int8_t DS3234_get_aging(const uint8_t pin);

// temperature register
float DS3234_get_treg(const uint8_t pin);

// alarms
void DS3234_set_a1(const uint8_t pin, const uint8_t s, const uint8_t mi, const uint8_t h, const uint8_t d,
                   const uint8_t * flags);
void DS3234_get_a1(const uint8_t pin, char *buf, const uint8_t len);
void DS3234_clear_a1f(const uint8_t pin);
uint8_t DS3234_triggered_a1(const uint8_t pin);

void DS3234_set_a2(const uint8_t pin, const uint8_t mi, const uint8_t h, const uint8_t d,
                   const uint8_t * flags);
void DS3234_get_a2(const uint8_t pin, char *buf, const uint8_t len);
void DS3234_clear_a2f(const uint8_t pin);
uint8_t DS3234_triggered_a2(const uint8_t pin);

// sram
void DS3234_set_sram_8b(const uint8_t pin, const uint8_t address, const uint8_t value);
uint8_t DS3234_get_sram_8b(const uint8_t pin, const uint8_t address);

// helpers
uint8_t dectobcd(const uint8_t val);
uint8_t bcdtodec(const uint8_t val);
uint8_t inp2toi(const char *cmd, const uint16_t seek);

/**
 * DS3232RTC Class
 */
class DS3234RTC
{
  public:
    DS3234RTC( uint8_t pin );
    DS3234RTC( uint8_t pin, const uint8_t ctrl_reg );
    static bool available();
    // Date and Time
    time_t get();
    static void set(time_t t);
    void read(tmElements_t &tm);
    static void write(tmElements_t &tm);
    static void writeTime(tmElements_t &tm);
    static void writeDate(tmElements_t &tm);
    // Alarms
    static void readAlarm(uint8_t alarm, alarmMode_t &mode, tmElements_t &tm);
    static void writeAlarm(uint8_t alarm, alarmMode_t mode, tmElements_t tm);
    // Control Register
    static void setBBOscillator(bool enable);
    static void setBBSqareWave(bool enable);
    static void setSQIMode(sqiMode_t mode);
    static bool isAlarmInterupt(uint8_t alarm);
    static uint8_t readControlRegister();
    static uint8_t readStatusRegister();
    // Control/Status Register
    static bool isOscillatorStopFlag();
    static void setOscillatorStopFlag(bool enable);
    static void setBB33kHzOutput(bool enable);
    static void setTCXORate(tempScanRate_t rate);
    static void set33kHzOutput(bool enable);
    static bool isTCXOBusy();
    static bool isAlarmFlag(uint8_t alarm);
    static uint8_t isAlarmFlag();
    static void clearAlarmFlag(uint8_t alarm);
    // Temperature
    static void readTemperature(tpElements_t &tmp);
  private:
    static uint8_t dec2bcd(uint8_t num);
    static uint8_t bcd2dec(uint8_t num);
    uint8_t SS_PIN;
  protected:
    static void _wTime(tmElements_t &tm);
    static void _wDate(tmElements_t &tm);
    static uint8_t read1(uint8_t addr);
    static void write1(uint8_t addr, uint8_t data);
};


#endif

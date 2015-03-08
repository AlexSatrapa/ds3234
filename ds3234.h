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
#define DS3234_A1F      0x01
#define DS3234_A2F      0x02
#define DS3234_BSY      0x04
#define DS3234_EN33KHZ  0x08
#define DS3234_CRATE0   0x10
#define DS3234_CRATE1   0x20
#define DS3234_BB33KHZ  0x40
#define DS3234_OSF      0x80

// helpers
uint8_t dectobcd(const uint8_t val);
uint8_t bcdtodec(const uint8_t val);

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
    void write(tmElements_t &tm);
    void writeTime(tmElements_t &tm);
    void writeDate(tmElements_t &tm);
    // Alarms
    void readAlarm(uint8_t alarm, alarmMode_t &mode, tmElements_t &tm);
    void writeAlarm(uint8_t alarm, alarmMode_t mode, tmElements_t tm);
    // Control Register
    void setBBOscillator(bool enable);
    void setBBSqareWave(bool enable);
    void setSQIMode(sqiMode_t mode);
    bool isAlarmInterrupt(uint8_t alarm);
    uint8_t readControlRegister();
    void writeControlRegister(uint8_t value);
    // Status Register
    bool isOscillatorStopFlag();
    void setOscillatorStopFlag(bool enable);
    void setBB33kHzOutput(bool enable);
    void setTCXORate(tempScanRate_t rate);
    void set33kHzOutput(bool enable);
    bool isTCXOBusy();
    bool isAlarmFlag(uint8_t alarm);
    uint8_t isAlarmFlag();
    void clearAlarmFlag(uint8_t alarm);
    uint8_t readStatusRegister();
    void writeStatusRegister(uint8_t value);
    // Temperature
    void readTemperature(tpElements_t &tmp);
    SPISettings spi_settings;
  private:
    uint8_t dec2bcd(uint8_t num);
    uint8_t bcd2dec(uint8_t num);
    uint8_t ss_pin;
  protected:
    uint8_t read1(uint8_t addr);
    void write1(uint8_t addr, uint8_t data);
};


#endif

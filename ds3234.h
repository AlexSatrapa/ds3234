#ifndef __ds3234_h_
#define __ds3234_h_

#if ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include <Time.h>
#include <DSRTC.h>

#define DS3234_WRITE 0x80

class DS3234RTC: public DSRTC
{
  public:
    DS3234RTC( uint8_t pin );
    DS3234RTC( uint8_t pin, const uint8_t ctrl_reg );
    SPISettings spi_settings;
  private:
    uint8_t ss_pin;
  protected:
    uint8_t read1(uint8_t addr);
    void write1(uint8_t addr, uint8_t data);
    void readN(uint8_t addr, uint8_t buf[], uint8_t len);
    void writeN(uint8_t addr, uint8_t buf[], uint8_t len);
};


#endif

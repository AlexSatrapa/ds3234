#ifndef __ds3234_h_
#define __ds3234_h_

#if ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include <Time.h>
#include <DSRTC.h>
#include <SPI.h>

#define DS3234_WRITE 0x80

// helpers
uint8_t dectobcd(const uint8_t val);
uint8_t bcdtodec(const uint8_t val);

class DS3234: public DSRTC
{
  public:
    DS3234( uint8_t pin );
    DS3234( uint8_t pin, const uint8_t ctrl_reg );
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
// -*- coding: utf-8; tab-width: 4; indent-tabs-mode: nil -*-

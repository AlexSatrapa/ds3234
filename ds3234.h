#ifndef __ds3234_h_
#define __ds3234_h_

#if ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include <DSRTC.h>
#include <SPI.h>

#define DS3234_WRITE 0x80

// helpers
byte dectobcd(const byte val);
byte bcdtodec(const byte val);

class DS3234: public DSRTC
{
  public:
    DS3234( byte pin );
    DS3234( byte pin, const byte ctrl_reg );
    SPISettings spi_settings;
  private:
    byte ss_pin;
  protected:
    byte read1(byte addr);
    void write1(byte addr, byte data);
    void readN(byte addr, byte buf[], byte len);
    void writeN(byte addr, byte buf[], byte len);
};

#endif
// -*- coding: utf-8; tab-width: 4; indent-tabs-mode: nil -*-

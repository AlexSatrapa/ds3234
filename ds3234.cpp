#include <ds3234.h>

DS3234::DS3234( byte pin )
{
	ss_pin = pin;
	pinMode(ss_pin, OUTPUT);
	SPI.begin();
	spi_settings = SPISettings(4000000, MSBFIRST, SPI_MODE1);
}

DS3234::DS3234( byte pin, const byte ctrl_reg )
{
	ss_pin = pin;
	pinMode(ss_pin, OUTPUT);
	spi_settings = SPISettings(4000000, MSBFIRST, SPI_MODE1);
}

byte DS3234::read1(byte addr)
{
	byte data;

	SPI.beginTransaction(spi_settings);
	digitalWrite(ss_pin, LOW);
	SPI.transfer(addr);
	data = SPI.transfer(0x00);
	digitalWrite(ss_pin, HIGH);
	SPI.endTransaction();
	return data;
}

void DS3234::write1(byte addr, byte value)
{
	SPI.beginTransaction(spi_settings);
	digitalWrite(ss_pin, LOW);
	SPI.transfer(addr + 0x80);
	SPI.transfer(value);
	digitalWrite(ss_pin, HIGH);
	SPI.endTransaction();
}

void DS3234::readN(byte addr, byte buf[], byte len)
{
	byte i;

	SPI.beginTransaction(spi_settings);
	digitalWrite(ss_pin, LOW);
	SPI.transfer(addr);
	for (i=0; i<len; i++)
	{
		buf[i] = SPI.transfer(0x00);
	}
	digitalWrite(ss_pin, HIGH);
	SPI.endTransaction();
}

void DS3234::writeN(byte addr, byte buf[], byte len)
{
	byte i;

	SPI.beginTransaction(spi_settings);
	digitalWrite(ss_pin, LOW);
	SPI.transfer(addr + 0x80);
	for (i=0; i<len; i++)
	{
		SPI.transfer(buf[i]);
	}
	digitalWrite(ss_pin, HIGH);
	SPI.endTransaction();
}

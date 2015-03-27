#include <ds3234.h>

DS3234::DS3234( uint8_t pin )
{
	ss_pin = pin;
	pinMode(ss_pin, OUTPUT);
	SPI.begin();
	spi_settings = SPISettings(4000000, MSBFIRST, SPI_MODE1);
}

DS3234::DS3234( uint8_t pin, const uint8_t ctrl_reg )
{
	ss_pin = pin;
	pinMode(ss_pin, OUTPUT);
	spi_settings = SPISettings(4000000, MSBFIRST, SPI_MODE1);
}

uint8_t DS3234::read1(uint8_t addr)
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

void DS3234::write1(uint8_t addr, uint8_t value)
{
	SPI.beginTransaction(spi_settings);
	digitalWrite(ss_pin, LOW);
	SPI.transfer(addr + 0x80);
	SPI.transfer(value);
	digitalWrite(ss_pin, HIGH);
	SPI.endTransaction();
}

void DS3234::readN(uint8_t addr, uint8_t buf[], uint8_t len)
{
	uint8_t i;

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

void DS3234::writeN(uint8_t addr, uint8_t buf[], uint8_t len)
{
	uint8_t i;

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

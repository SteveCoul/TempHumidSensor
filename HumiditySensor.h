#ifndef __HumiditySensor_hpp__
#define __HumiditySensor_hpp__

class HumiditySensor {
public:
	static void reset();
	static bool read( uint16_t* raw_temp, uint16_t* raw_humid );
	static void update();
	static void sensorHeat( bool on_or_off );
private:
	static uint8_t crc8( uint8_t *datptr, uint8_t len );
	static void i2cCmd( uint16_t cmd );
};

#endif



#include <HumiditySensor.h>

uint8_t HumiditySensor::crc8( uint8_t *datptr, uint8_t len ) {
	uint8_t crc = 0xFF;
	for(int j = len; j; j--) {
		crc ^= *datptr++;
		for(int i = 8; i; i--) {
			if(crc & 0x80) crc = (crc << 1) ^ 0x31;
			else crc = (crc << 1);
		}
	}
	return crc;
}

void HumiditySensor::i2cCmd( uint16_t cmd ) {
	Wire.beginTransmission( I2C_ADDRESS );
	Wire.write(cmd>>8);
	Wire.write(cmd & 0xFF);
	Wire.endTransmission();
}

void HumiditySensor::sensorHeat( bool on_or_off ) {
	i2cCmd( on_or_off ? 0x306D : 0x3066 );
}

void HumiditySensor::reset() {
	i2cCmd( 0x30A2 );
	delay(2);
}

bool HumiditySensor::read( uint16_t* raw_temp, uint16_t* raw_humid ) {
	uint8_t data_arr[6];

	i2cCmd( 0x2400 );
	delay(15);  
  
	Wire.requestFrom( (uint8_t)I2C_ADDRESS, (uint8_t)6);
	if( Wire.available() != 6 ) return false; 
  
	for(uint8_t i=0; i<6; i++) 
    	data_arr[i] = Wire.read();

	raw_temp[0] = (data_arr[0] << 8) | data_arr[1];
	if(data_arr[2] != crc8(data_arr, 2)) return false;

	raw_humid[0] = (data_arr[3] << 8) | data_arr[4];
	if(data_arr[5] != crc8(data_arr+3, 2)) return false;

	return true;
}

void HumiditySensor::update() {
	uint16_t t;
	uint16_t h;
	if ( HumiditySensor::read( &t, &h ) == false ) {
		currentTemperature = 0.0f;
		currentHumidity = 0.0f;
		Serial.println("Failed to read sensor");
	} else {
		float temp = (float)t * 315.0 / 65536.0 - 49.0;
		float humd = (float)h * 100.0 / 65536.0;
		add_sample( temp, humd );
	}
}




#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h> 
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h> 

#define I2C_ADDRESS	0x44

#include "secret.h"

/* ********************************************************** *
 *
 * ********************************************************** */

ESP8266WiFiMulti wifi;
ESP8266WebServer server(80);

float currentTemperature;
float currentHumidity;

/* ********************************************************** *
 *
 * ********************************************************** */

uint8_t crc8( uint8_t *datptr, uint8_t len ) {
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

void i2cCmd( uint16_t cmd ) {
	Wire.beginTransmission( I2C_ADDRESS );
	Wire.write(cmd>>8);
	Wire.write(cmd & 0xFF);
	Wire.endTransmission();
}

void sensorReset() {
	i2cCmd( 0x30A2 );
	delay(2);
}

void sensorHeat( bool on_or_off ) {
	i2cCmd( on_or_off ? 0x306D : 0x3066 );
}

bool sensorRead( uint16_t* raw_temp, uint16_t* raw_humid ) {
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

void sensorUpdate() {
	uint16_t t;
	uint16_t h;
	if ( sensorRead( &t, &h ) == false ) {
		currentTemperature = 0.0f;
		currentHumidity = 0.0f;
		Serial.println("Failed to read sensor");
	} else {
		currentTemperature = (float)t * 315.0 / 65536.0 - 49.0;
		currentHumidity = (float)h * 100.0 / 65536.0;
		Serial.print( currentTemperature );
		Serial.print( "F, humidity " );
		Serial.print( currentHumidity );
		Serial.println( "%" );
	}
}

/* ********************************************************** *
 *
 * ********************************************************** */

void handleNotFound(){
	server.send(404, "text/plain", "404: Not found");
}
 
/* ********************************************************** *
 *
 * ********************************************************** */

void setup() {
	Serial.begin( 115200 );
	Serial.println("Startup");

	Wire.begin();

	sensorReset();

	wifi.addAP( SECRET_SSID, SECRET_PASSWORD );

	Serial.println( SECRET_SSID );
	Serial.println( SECRET_PASSWORD );

	while (wifi.run() != WL_CONNECTED) {
    	delay(250);
    	Serial.print('.');
	}
  	Serial.print("Connected to ");
  	Serial.println(WiFi.SSID());
  	Serial.print("IP address:\t");
  	Serial.println(WiFi.localIP());

  	if (MDNS.begin("esp8266")) { 
    	Serial.println("mDNS responder started");
	} else {
    	Serial.println("Error setting up MDNS responder!");
	}

	server.onNotFound(handleNotFound); 
	server.begin();

	MDNS.addService("_http", "_tcp", 80);
}

void loop() {
	static int counter = 0;
	delay(100);
	counter++;

	MDNS.update();
	server.handleClient();

	if ( counter == 20 ) {
		counter = 0;
		sensorUpdate();
	}
}


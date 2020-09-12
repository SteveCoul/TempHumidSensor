
#include <EEPROM.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h> 
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h> 
#include <ArduinoOTA.h>

#define I2C_ADDRESS	0x44

#include "secret.h"

/* ********************************************************** *
 *
 * ********************************************************** */

ESP8266WiFiMulti wifi;
ESP8266WebServer server(80);
WiFiServer dataserver(32768);

char location[64];
float currentTemperature;
float currentHumidity;

/* ********************************************************** *
 *
 * ********************************************************** */

void readLocation() {
	for ( int i = 0; i < 64; i++ )
		location[i] = (char)EEPROM.read( i );
}

void setLocation( const char* name ) {
	strcpy( location, name );
	for ( int i = 0; i < 64; i++ )
		EEPROM.write( i, location[i] );
	if ( ! EEPROM.commit()  ) {
		Serial.println("Failed to commit eeprom");
	}
	MDNS.addServiceTxt("temphumidsensor", "tcp", "location", location );
}

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

char tmp[1024];

void handleNotFound(){
	server.send(404, "text/plain", "404: Not found");
}

void handleRoot() {
	char* p = tmp;
	p+=sprintf(p,"<HTML>");
	p+=sprintf(p,"<BODY>");
	p+=sprintf(p,"<P>One of my temperature sensors</P>");
	p+=sprintf(p,"<P>Called %s</P>",  WiFi.softAPSSID().c_str() );
	p+=sprintf(p,"<P>Location %s</P>", location );
	p+=sprintf(p,"<P>Temperature %f</P>", currentTemperature );
	p+=sprintf(p,"<P>Humidity %f</P>", currentHumidity );
	p+=sprintf(p,"</BODY>");
	p+=sprintf(p,"</HTML>");
	server.send(200,"text/html", tmp );
}
 
void handleData() {
	char* p = tmp;
	p+=sprintf( p, "{" );
	p+=sprintf( p, "\"data\": {");
	p+=sprintf( p, "\"name\": \"%s\",", WiFi.softAPSSID().c_str() );
	p+=sprintf( p, "\"location\": \"%s\",", location );
	p+=sprintf( p, "\"temperature\": \"%f\",", currentTemperature );
	p+=sprintf( p, "\"humidity\": \"%f\"", currentHumidity );
	p+=sprintf( p, "}");
	p+=sprintf( p, "}");
	server.send(200,"text/json", tmp );
}

void handleSetLocation() {

	const char* arg = server.arg("arg").c_str();
	setLocation( arg );

	char* p = tmp;
	p+=sprintf(p,"<HTML>");
	p+=sprintf(p,"<BODY>");
	p+=sprintf(p,"<P>Location now %s</P>", location );
	p+=sprintf(p,"</BODY>");
	p+=sprintf(p,"</HTML>");
	server.send(200,"text/html", tmp );
}

/* ********************************************************** *
 *
 * ********************************************************** */

void setup() {

	Serial.begin( 115200 );
	Serial.println("\nStartup");

	Serial.println("Storage Start");
	EEPROM.begin( 512 );

	Serial.println("Read Location");
	readLocation();

	Serial.println("Set SSID");
	WiFi.softAP( "FIXME" );

	Serial.print("ConfigAP SSID: ");
	Serial.println( WiFi.softAPSSID() );

	Serial.println("Start i2c");
	Wire.begin();

	Serial.println("Reset sensor");
	sensorReset();

	Serial.println("Add AP");	
	wifi.addAP( SECRET_SSID, SECRET_PASSWORD );

	Serial.println("Wait for WiFi");
	while (wifi.run() != WL_CONNECTED) {
    	delay(250);
    	Serial.print('.');
	}
  	Serial.print("Connected to ");
  	Serial.println(WiFi.SSID());
  	Serial.print("IP address:\t");
  	Serial.println(WiFi.localIP());

	configTime(0, 0, "pool.ntp.org", "time.nist.gov");

	ArduinoOTA.setPassword( SECRET_PASSWORD );

  	if (MDNS.begin(  WiFi.softAPSSID().c_str(), WiFi.localIP())) { 
    	Serial.println("mDNS responder started");
		MDNS.addService("http", "tcp", 80);
		MDNS.addService("temphumidsensor", "tcp", 32768);
		MDNS.addServiceTxt("temphumidsensor", "tcp", "location", location );
	} else {
    	Serial.println("Error setting up MDNS responder!");
	}

	Serial.println("OTA");
	ArduinoOTA.begin();

	Serial.println("HTTP");
	server.on("/", handleRoot);
	server.on("/data.json", handleData);
	server.on("/setLocation", handleSetLocation);
	server.onNotFound(handleNotFound); 
	server.begin();

	Serial.println("DATA");
	dataserver.begin();
}

void loop() {
	static int counter = 0;
	delay(100);
	counter++;

	ArduinoOTA.handle();
	MDNS.update();
	server.handleClient();

	WiFiClient c = dataserver.available();
	if ( c ) {
		c.println("{");
		c.println("  \"data\": {");
		c.print("    \"name\": \""); c.print( WiFi.softAPSSID() ); c.println("\",");
		c.print("    \"location\": \""); c.print( location ); c.println("\",");
		c.print("    \"temperature\": \""); c.print( currentTemperature ); c.println("\",");
		c.print("    \"humidity\": \""); c.print( currentHumidity ); c.println("\"");
		c.println("  }");
		c.println("}");
		c.flush();
		c.stop();
	}

	if ( counter == 20 ) {
	  	time_t current = time(nullptr);
		Serial.println(ctime(&current));
		counter = 0;
		sensorUpdate();
	}
}


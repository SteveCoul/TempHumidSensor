

#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h> 
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h> 
#include <ArduinoOTA.h>

#include <HumiditySensor.h>

#define I2C_ADDRESS	0x44

#include "secret.h"

/* Millisecond delay in main poll loop */
#define	POLL_DELAY			100

/* How many seconds between reading sensor */
#define SAMPLE_SPACING		2

/* We do a rolling average of *this* many samples */
#define AVERAGE_DEPTH		30

/* How often (#samples added) before recording a record */
/* Needs to be <= average depth really */
#define	RECORD_GAP			3

/* History Depth 
   ( history is time ( 32bit ) and our two 16 bit values
	 in 8.8 arithmatic. So it's 8 bytes per sample

	 a 1k buffer could handle 128 samples ) */

//#define HISTORY				2560 /* 128 = 20k = 40 hours */
#define HISTORY 5

/* ********************************************************** *
 *
 * ********************************************************** */

#define MIN(a,b)	( (a<b)?(a):(b) )

/* ********************************************************** *
 *
 * ********************************************************** */

uint32_t device_id() {
	uint8_t c[6];
	WiFi.macAddress( c );
	
	c[0] = c[0] ^ c[1] ^ c[2];

	return ( (c[0]<<24) | (c[3]<<16) | (c[4]<<8) | c[5] );
}

/* ********************************************************** *
 *
 * ********************************************************** */

const char* device_name() {
	static char name[16+8+1] = { 0 };
	if ( name[0] == '\0' ) {
		(void)sprintf( 	name, 
						"TempHumidSensor-%08X", device_id() );
	}
	return (const char*)name;
}

/* ********************************************************** *
 *
 * ********************************************************** */

ESP8266WiFiMulti wifi;
ESP8266WebServer server(80);
WiFiServer dataserver(32768);

/* ********************************************************** *
 *
 * ********************************************************** */

uint32_t history[ HISTORY*2 ];
size_t history_ptr = 0;
size_t history_count = 0;

uint32_t compressValues( float t, float h ) {
	unsigned int T = (unsigned int)( t * 256 );
	unsigned int H = (unsigned int)( h * 256 );
	T = T & 0xFFFF;
	H = H & 0xFFFF;
	Serial.print("  compress ");
	Serial.print(t);
	Serial.print(" and ");
	Serial.print(h);
	Serial.print(" via 0x");
	Serial.print(T,HEX);
	Serial.print(" and 0x");
	Serial.println(H,HEX);
	return ( T << 16 ) | H;
}

void decompressValues( uint32_t raw, float* pt, float* ph ) {
	pt[0] = (float)((raw>>16)&0xFFFF);
	ph[0] = (float)(raw&0xFFFF);
	pt[0]/=256.0;
	ph[0]/=256.0;
}

void save_history( float temp, float humid ) {
	time_t current = time(nullptr);

	Serial.print(ctime(&current));
	Serial.print(" saving history of ");
	Serial.print(temp);
	Serial.print(" degrees @ ");
	Serial.print(humid);
	Serial.println("% RH");
	
	history[ (history_ptr*2)+0 ] = (uint32_t)current;
	history[ (history_ptr*2)+1 ] = compressValues( temp, humid );

	history_ptr = ( history_ptr + 1 ) % HISTORY;
	history_count = MIN( history_count+1, HISTORY );
}

char json_tmp[512];

const char* json_history_record( size_t idx ) {
	unsigned int T;
	float t;
	float h;
	size_t p = ( history_ptr - history_count + idx ) % HISTORY;
	T = history[ (p*2)+0 ];
	decompressValues( history[(p*2)+1], &t, &h );
	(void)sprintf( json_tmp, 
		"{\"time\": \"%u\",\"temperature\": \"%f\",\"humidity\": \"%f\"}",
		T, t, h );
	return (const char*) json_tmp;
}

/* ********************************************************** *
 *
 * ********************************************************** */

float currentTemperature;
float currentHumidity;

float avg_temperature[ AVERAGE_DEPTH ];
float avg_humidity[ AVERAGE_DEPTH ];
size_t avg_ptr = 0;
size_t avg_count = 0;
size_t added = 0;

void add_sample( float t, float h ) {
	avg_temperature[ avg_ptr ] = t;
	avg_humidity[ avg_ptr ] = h;
	avg_ptr = ( avg_ptr + 1 ) % AVERAGE_DEPTH;
	currentTemperature = 0.0;
	currentHumidity = 0.0;
	if ( avg_count < AVERAGE_DEPTH ) avg_count++;

	for ( int i = 0; i < avg_count; i++ ) {
		currentTemperature += avg_temperature[ i ];
		currentHumidity += avg_humidity[ i ];
	}

	currentTemperature/=avg_count;
	currentHumidity/=avg_count;

	Serial.print("Currently ");
	Serial.print(t);
	Serial.print(" degrees, ");
	Serial.print(h);
	Serial.print(" %% R-humidity. Rolling average of ");
	Serial.print(currentTemperature);
	Serial.print(" degress, ");
	Serial.print(currentHumidity);
	Serial.print(" %% R-humidity");
	Serial.println("");

	added++;
	if ( added == RECORD_GAP ) {
		added = 0;
		save_history( currentTemperature, currentHumidity );
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
	p+=sprintf( p, "\"temperature\": \"%f\",", currentTemperature );
	p+=sprintf( p, "\"humidity\": \"%f\"", currentHumidity );
	p+=sprintf( p, "}");
	p+=sprintf( p, "}");
	server.send(200,"text/json", tmp );
}

/* ********************************************************** *
 *
 * ********************************************************** */

void setup() {

	Serial.begin( 115200 );
	Serial.println("\nStartup");

	Serial.print("Set SSID "); Serial.println( device_name() );
	WiFi.softAP( device_name() );

	Serial.print("ConfigAP SSID: ");
	Serial.println( WiFi.softAPSSID() );

	Serial.println("Start i2c");
	Wire.begin();

	Serial.println("Reset sensor");
	HumiditySensor::reset();

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
	} else {
    	Serial.println("Error setting up MDNS responder!");
	}

	Serial.println("OTA");
	ArduinoOTA.begin();

	Serial.println("HTTP");
	server.on("/", handleRoot);
	server.on("/data.json", handleData);
	server.onNotFound(handleNotFound); 
	server.begin();

	Serial.println("DATA");
	dataserver.begin();
}

void loop() {
	static int counter = 0;
	delay(POLL_DELAY);
	counter++;

	ArduinoOTA.handle();
	MDNS.update();
	server.handleClient();

	WiFiClient c = dataserver.available();
	if ( c ) {
		for ( int i = 0; i < history_count; i++ ) {
			c.println( json_history_record( i ) );
		}
		c.flush();
		c.stop();
	}

	if ( counter == ((SAMPLE_SPACING*1000)/POLL_DELAY) ) {
		counter = 0;
		HumiditySensor::update();
	}
}


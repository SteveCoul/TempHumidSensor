
all:
	echo "#define SECRET_SSID \"$(SSID)\"" > secret.h
	echo "#define SECRET_PASSWORD \"$(PASSWORD)\"" >> secret.h
	echo "static const unsigned char appdata[] = { \\" > app.html.h
	( cat app.html && echo "\0") | xxd -i >> app.html.h
	echo "};" >> app.html.h
	./ArduinoDockerBuildMachine/arduino_wemos_miniD1_build.sh 
upload:
	python ./build/esptool.py --chip auto --port /dev/cu.usbserial* write_flash --verify --flash_mode=dio --flash_size=16m 0x00000 build/esp8266.esp8266.d1/TempHumidSensor.ino.bin

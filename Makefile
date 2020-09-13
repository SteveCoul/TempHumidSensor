
all:
	echo "#define SECRET_SSID \"$(SSID)\"" > secret.h
	echo "#define SECRET_PASSWORD \"$(PASSWORD)\"" >> secret.h
	./ArduinoDockerBuildMachine/arduino_wemos_miniD1_build.sh 
upload:
	python ./esptool.py --chip auto --port /dev/cu.usbserial* write_flash --verify --flash_mode=dio --flash_size=16m 0x00000 build/esp8266.esp8266.d1/TempHumidSensor.ino.bin

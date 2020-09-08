
ARDUINO_CLI=$(shell which arduino-cli)
ARDUINO_PLATFORM="esp8266:esp8266:d1"

all:
	$(ARDUINO_CLI) compile -v --build-path="$(PWD)/tmp" -b $(ARDUINO_PLATFORM) --build-properties "compiler.cpp.extra_flags=-I." 

upload:
	python ./esptool.py --chip auto --port /dev/cu.usbserial* write_flash --verify --flash_mode=dio --flash_size=16m 0x00000 TempHumidSensor.esp8266.esp8266.d1.bin

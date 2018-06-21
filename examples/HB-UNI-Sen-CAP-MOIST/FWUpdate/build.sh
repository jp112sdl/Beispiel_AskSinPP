#!/bin/bash
usage() { 
	echo "Usage: $0 -s SERIAL [-c -b -f -h]"
	echo "-s				10 digit device serial"
	echo "-c				compile sketch to a .hex-file"
	echo "-b				create bootloader and flash via usbasp"
	echo "-f /dev/cu.wchusbserial1420	create .eq3 firmware file and flash via ota"
	echo "-h				show this help"
	exit 1
	}

while getopts ":s:bcf:" o; do
    case "${o}" in
        s)
            SERIAL=${OPTARG}
            ;;
        b)
	    CREATE_BOOTLOADER=true
            ;;
        f)
            FLASH_OTA=${OPTARG}
            ;;
        c)  
            COMPILE=true
            ;;
        *)
            usage
            ;;
    esac
done
shift $((OPTIND-1))

if [ ${#SERIAL} != 10 ]; then
 	printf "\nERROR: SERIAL must have 10 digits\n\n"
 	exit 4
fi

ID=${SERIAL:8:2}  
DEVICE_TYPE=f311
SKETCH=HB-UNI-Sen-CAP-MOIST

echo "SERIAL  = ${SERIAL}"
echo "ID      = ${ID}"
echo "DEVTYPE = ${DEVICE_TYPE}"
echo "SKETCH  = ${SKETCH}"
echo ""

if [ "${COMPILE}" ]; then
	BUILD_PATH=/var/folders/q_/pltbkm613qzdp54tz13stm5h0000gp/T/arduino_build_444073
        BUILD_CACHE=/var/folders/q_/pltbkm613qzdp54tz13stm5h0000gp/T/arduino_cache_309895
	CURRENT_DIR=`pwd`
	cd ../${SKETCH}
	echo "Enabling #define USE_OTA_BOOTLOADER in Sketch..."
	sed -i "s/\/\/ #define USE_OTA_BOOTLOADER/#define USE_OTA_BOOTLOADER/g" ${SKETCH}.ino

	mkdir -p ${BUILD_PATH}
	mkdir -p ${BUILD_CACHE}

	echo "Compiling Sketch..."
	/Applications/Arduino-1.8.5.app/Contents/Java/arduino-builder \
	-compile \
	-hardware /Applications/Arduino-1.8.5.app/Contents/Java/hardware \
	-hardware ~/Library/Arduino15/packages \
	-tools /Applications/Arduino-1.8.5.app/Contents/Java/tools-builder \
	-tools /Applications/Arduino-1.8.5.app/Contents/Java/hardware/tools/avr \
	-tools ~/Library/Arduino15/packages \
	-built-in-libraries /Applications/Arduino-1.8.5.app/Contents/Java/libraries \
	-libraries ~/Documents/Arduino/libraries \
	-fqbn=arduino:avr:pro:cpu=8MHzatmega328 \
	-ide-version=10805 \
	-build-path $BUILD_PATH \
	-warnings=none \
	-build-cache ${BUILD_CACHE} \
	-prefs=build.warn_data_percentage=75 \
	-prefs=runtime.tools.avr-gcc.path=/Users/${USER}/Library/Arduino15/packages/arduino/tools/avr-gcc/4.9.2-atmel3.5.4-arduino2 \
	-prefs=runtime.tools.avrdude.path=~/Library/Arduino15/packages/arduino/tools/avrdude/6.3.0-arduino9 \
	-prefs=runtime.tools.arduinoOTA.path=~/Library/Arduino15/packages/arduino/tools/arduinoOTA/1.1.1 \
	./${SKETCH}.ino

	echo "Disabling #define USE_OTA_BOOTLOADER in Sketch..."
	sed -i "s/#define USE_OTA_BOOTLOADER/\/\/ #define USE_OTA_BOOTLOADER/g" ${SKETCH}.ino

	mv -f $BUILD_PATH/${SKETCH}.ino.hex ./${SKETCH}-OTA-FW.hex
	cd ${CURRENT_DIR}
fi

if [ "${CREATE_BOOTLOADER}" ]; then
	echo "Creating bootloader for $1"
	./makeota.sh ${DEVICE_TYPE} ${DEVICE_TYPE}${ID} ${SERIAL} > ${SERIAL}_BOOTLOADER.hex
	./flash.sh ${SERIAL}_BOOTLOADER.hex
fi

if [ "${FLASH_OTA}" ]; then
	rm update.tar.gz
	cd src/
	rm *.eq3
	echo "Creating .eq3 firmware file"
	../prepota.sh ../../${SKETCH}/${SKETCH}-OTA-FW.hex 
	tar czf ../update.tar.gz *
	cd ..
	echo "Flashing firmware via OTA on ${FLASH_OTA}"
	./flash-ota -f src/${SKETCH}-OTA-FW_*.eq3 -s ${SERIAL} -U ${FLASH_OTA}
fi

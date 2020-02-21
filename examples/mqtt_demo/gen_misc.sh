#!/bin/bash



echo "gen_misc.sh version 20180211 edit by tuanlt"

echo ""



export BIN_PATH=/mnt/Share/ESP8266_BIN

dev=/dev/ttyUSB0

baud=74880

boot=new

echo "boot mode: $boot"

app=1

echo "app:$app"

spi_speed=40

echo "spi speed: $spi_speed MHz"

spi_mode=DOUT

echo "spi mode: $spi_mode"

spi_size_map=6

flash=512



echo "spi_size_map:$spi_size_map"



if [ $spi_size_map == 2 ]; then

    flash=1024



elif [ $spi_size_map == 3 ]; then

	flash=2048

elif [ $spi_size_map == 4 ]; then

	flash=4096

elif [ $spi_size_map == 5 ]; then

	flash=2048

elif [ $spi_size_map == 6 ]; then

	flash=4096



fi



make clean

make COMPILE=gcc BOOT=$boot APP=$app SPI_SPEED=$spi_speed SPI_MODE=$spi_mode SPI_SIZE_MAP=$spi_size_map

bin="user$app.$flash.$boot.$spi_size_map.bin"
#cp /mnt/Share/GRATIOT_WALL/bin/upgrade/$bin $BIN_PATH/upgrade/$bin
echo "Please enter Y or y go to flash device:"

read input



if [[ $input != Y ]] && [[ $input != y ]]; then

    exit

fi
sudo esptool.py --port $dev write_flash --flash_mode dout --flash_size 4MB-c1 0x0 $BIN_PATH/boot_v1.6.bin 0x01000 $BIN_PATH/upgrade/$bin



echo "Please enter Y or y go to console terminal board:"

read input



if [[ $input != Y ]] && [[ $input != y ]]; then

    exit

fi

echo "Going to terminal console test board after 5s, to exit press Ctrl+]"

sleep 1

sudo python -m serial.tools.miniterm $dev $baud



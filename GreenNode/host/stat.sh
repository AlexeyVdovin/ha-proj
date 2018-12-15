#!/bin/bash


I2C_RD=/home/av/ha-proj/GreenNode/host/i2c-rd

PIZ_3V3=$(${I2C_RD} 0)
PIZ_5V0=$(${I2C_RD} 2)
OW_3V3=$(${I2C_RD} 4)
VCC_12V=$(${I2C_RD} 12)
ACDC_12V=$(${I2C_RD} 14)
STATUS=$(${I2C_RD} 18)

URL="http://10.8.0.1/greenhouse/stat.php?piz3v3=${PIZ_3V3}&piz5v0=${PIZ_5V0}&ow3v3=${OW_3V3}&vcc12v=${VCC_12V}&acdc12v=${ACDC_12V}&status=${STATUS}"

curl -f -s -o /dev/null ${URL}

#echo $?

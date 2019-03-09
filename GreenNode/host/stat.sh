#!/bin/bash

# 0x28e81d7791170295 21625
# 0x28a51b779113020d 21062
# 0x28e00646920402a2 20750
# 0x2821cc46920302fb 20625

PHP=/usr/bin/php
I2C_RD=/home/av/ha-proj/GreenNode/host/i2c-rd
T_RD=/home/av/ha-proj/GreenNode/host/ds2482
MEM_WR=/home/av/ha-proj/GreenNode/host/mem_wr.php
MEM_RD=/home/av/ha-proj/GreenNode/host/mem_rd.php

${T_RD} 0x18 | while read n v
  do ${PHP} ${MEM_WR} $n $v
done

${T_RD} 0x1A | while read n v
  do ${PHP} ${MEM_WR} $n $v
done

PIZ_3V3=$(${I2C_RD} 0)
PIZ_5V0=$(${I2C_RD} 2)
OW_3V3=$(${I2C_RD} 4)
VCC_12V=$(${I2C_RD} 12)
ACDC_12V=$(${I2C_RD} 14)
STATUS=$(${I2C_RD} 18)
RELAY=$(${I2C_RD} 20)

URL="http://10.8.0.1/greenhouse/stat.php?piz3v3=${PIZ_3V3}&piz5v0=${PIZ_5V0}&ow3v3=${OW_3V3}&vcc12v=${VCC_12V}&acdc12v=${ACDC_12V}&status=${STATUS}&relay=${RELAY}" 

curl -f -s -o /dev/null ${URL}

#echo $?

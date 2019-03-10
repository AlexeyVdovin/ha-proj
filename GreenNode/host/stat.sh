#!/bin/bash

PHP=/usr/bin/php
REPO=/home/av/ha-proj/GreenNode/host
I2C_RD=${REPO}/i2c-rd
T_RD=${REPO}/ds2482
MEM_WR=${REPO}/mem_wr.php
MEM_RD=${REPO}/mem_rd.php
MEM_SET=${REPO}/mem_set.php
MEM_PROC=${REPO}/mem_proc.php

. ${REPO}/sensors.conf
${MEM_SET} G1_GROUND ${G1_GROUND}
${MEM_SET} G1_AIR ${G1_AIR}
${MEM_SET} G2_GROUND ${G2_GROUND}
${MEM_SET} G2_AIR ${G2_AIR}

${T_RD} 0x18 | while read n v
  do ${PHP} ${MEM_WR} $n $v
done

${T_RD} 0x1A | while read n v
  do ${PHP} ${MEM_WR} $n $v
done

${MEM_PROC}

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

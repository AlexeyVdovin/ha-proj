#!/bin/bash -x

PHP=/usr/bin/php
REPO=/home/av/ha-proj/GreenNode/host
I2C_RD=${REPO}/i2c-rd
I2C_WR=${REPO}/i2c-wr
T_RD=${REPO}/ds2482
MEM_WR=${REPO}/mem_wr.php
MEM_RD=${REPO}/mem_rd.php
MEM_SET=${REPO}/mem_set.php
MEM_PROC=${REPO}/mem_proc.php

. ${REPO}/sensors.conf
${PHP} ${MEM_SET} G1_GROUND ${G1_GROUND}
${PHP} ${MEM_SET} G1_GROUND_CORR ${G1_GROUND_CORR}
${PHP} ${MEM_SET} G1_AIR ${G1_AIR}
${PHP} ${MEM_SET} G1_AIR_CORR ${G1_AIR_CORR}
${PHP} ${MEM_SET} G2_GROUND ${G2_GROUND}
${PHP} ${MEM_SET} G2_GROUND_CORR ${G2_GROUND_CORR}
${PHP} ${MEM_SET} G2_AIR ${G2_AIR}
${PHP} ${MEM_SET} G2_AIR_CORR ${G2_AIR_CORR}

. /var/www/store/settings.conf
${PHP} ${MEM_SET} G1_FREEZ ${G1_FREEZ}
${PHP} ${MEM_SET} G1_VENT_C ${G1_VENT_C}
${PHP} ${MEM_SET} G1_HEAT_C ${G1_HEAT_C}
${PHP} ${MEM_SET} G1_CIRC_C ${G1_CIRC_C}
${PHP} ${MEM_SET} G2_FREEZ ${G2_FREEZ}
${PHP} ${MEM_SET} G2_VENT_C ${G2_VENT_C}
${PHP} ${MEM_SET} G2_HEAT_C ${G2_HEAT_C}
${PHP} ${MEM_SET} G2_CIRC_C ${G2_CIRC_C}

${T_RD} 0x18 > /tmp/0x18.tmp
rc=$?

if [ $rc != 0 ]; then
  # TODO: Send error to the server
  exit 0
fi

${T_RD} 0x1A > /tmp/0x1A.tmp
rc=$?

if [ $rc != 0 ]; then
  # TODO: Send error to the server
  exit 0
fi

cat /tmp/0x18.tmp | while read n v
  do ${PHP} ${MEM_WR} $n $v
done

cat /tmp/0x1A.tmp | while read n v
  do ${PHP} ${MEM_WR} $n $v
done

${PHP} ${MEM_PROC}

G1_HEAT=$(${PHP} ${MEM_RD} G1_HEAT 0)
G2_HEAT=$(${PHP} ${MEM_RD} G2_HEAT 0)
G1_VENT=$(${PHP} ${MEM_RD} G1_VENT 0)
G2_VENT=$(${PHP} ${MEM_RD} G2_VENT 0)
G1_CIRC=$(${PHP} ${MEM_RD} G1_CIRC 0)
G2_CIRC=$(${PHP} ${MEM_RD} G2_CIRC 0)

# FIXME: Set correct channel ID for actuators
${I2C_WR} 0x53 $((${G1_HEAT}+${G2_HEAT}))
${I2C_WR} 0x54 $((${G1_VENT}+${G2_VENT}))

${I2C_WR} 0x51 ${G1_CIRC}
${I2C_WR} 0x52 ${G2_CIRC}

ping -c 3 -n ya.ru
rc=$?

COUNT=0
# Simulate permanent PING fail # if [ $rc != 0 ]; then
  COUNT=$(${PHP} ${MEM_RD} PING_COUNT 0)
  COUNT=$((${COUNT}+1))
# fi
${PHP} ${MEM_SET} PING_COUNT ${COUNT}

if [ ${COUNT} -gt 30 ]; then
  echo "Do cold restart."
  shutdown -P now
fi

if [ $rc != 0 ]; then
  exit 0
fi

G1_GROUND_T=$(${PHP} ${MEM_RD} ${G1_GROUND} unknown)
G1_GROUND_TT=$(${PHP} ${MEM_RD} t_${G1_GROUND} unknown)
G1_AIR_T=$(${PHP} ${MEM_RD} ${G1_AIR} unknown)
G1_AIR_TT=$(${PHP} ${MEM_RD} t_${G1_AIR} unknown)
G2_GROUND_T=$(${PHP} ${MEM_RD} ${G2_GROUND} unknown)
G2_GROUND_TT=$(${PHP} ${MEM_RD} t_${G2_GROUND} unknown)
G2_AIR_T=$(${PHP} ${MEM_RD} ${G2_AIR} unknown)
G2_AIR_TT=$(${PHP} ${MEM_RD} t_${G2_AIR} unknown)

URL="http://10.8.0.1/greenhouse/stat.php?g1gr=${G1_GROUND_T}&g1grt=${G1_GROUND_TT}&g1ar=${G1_AIR_T}&g1art=${G1_AIR_TT}&g2gr=${G2_GROUND_T}&g2grt=${G2_GROUND_TT}&g2ar=${G2_AIR_T}&g2art=${G2_AIR_TT}&g1ht=${G1_HEAT}&g2ht=${G2_HEAT}&g1vt=${G1_VENT}&g2vt=${G2_VENT}&g1cr=${G1_CIRC}&g2cr=${G2_CIRC}" 

curl --retry 3 --retry-delay 5 --retry-max-time 30 --max-time 30 -f -s -o /dev/null ${URL}

PIZ_3V3=$(${I2C_RD} 0)
PIZ_5V0=$(${I2C_RD} 2)
OW_3V3=$(${I2C_RD} 4)
VCC_12V=$(${I2C_RD} 12)
ACDC_12V=$(${I2C_RD} 14)
STATUS=$(${I2C_RD} 18)
RELAY=$(${I2C_RD} 20)

URL="http://10.8.0.1/greenhouse/bmc.php?piz3v3=${PIZ_3V3}&piz5v0=${PIZ_5V0}&ow3v3=${OW_3V3}&vcc12v=${VCC_12V}&acdc12v=${ACDC_12V}&status=${STATUS}&relay=${RELAY}" 

curl --retry 3 --retry-delay 5 --retry-max-time 30 --max-time 30 -f -s -o /dev/null ${URL}

#echo $?

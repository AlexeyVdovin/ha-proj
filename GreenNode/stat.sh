#!/bin/bash -x

date

PHP=/usr/bin/php
REPO=/home/av/ha-proj/GreenNode
#I2C_RD=${REPO}/i2c-rd
#I2C_WR=${REPO}/i2c-wr
MEM_WR=${REPO}/mem_wr.php
MEM_RD=${REPO}/mem_rd.php
MEM_SET=${REPO}/mem_set.php

uptime -p > /tmp/uptime.txt

ping -c 3 -n ya.ru
rc=$?

# Simulate failed PING
#rc=1

COUNT=0
if [ $rc != 0 ]; then
  COUNT=$(${PHP} ${MEM_RD} PING_COUNT 0)
  COUNT=$((${COUNT}+1))
fi
${PHP} ${MEM_SET} PING_COUNT ${COUNT}

if [ ${COUNT} -gt 30 ]; then
  echo "Do cold restart."
  /sbin/shutdown -P now
fi

if [ $rc != 0 ]; then
  echo "Failed ping count: "${COUNT}
  exit 0
fi

# Continue if network is accessible

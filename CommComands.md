# Packet structure #

| id1 | 0xAA |
|:----|:-----|
| id2 | 0xC4 |
| to | 00 - broadcast |
| from | Node ID |
| via | Route ID |
| flags | TBD |
| seq | Sequence number |
| len | Length of data |
| data`[]` | Max 16 bytes |
| crc16 | XMODEM CRC-16 |

# Command set #

## Commands: ##
  * 00 - set slave address `[00,ID,MAC[6]]`
  * 01 - response code `[01,{00-OK, 01-FAIL, 02-NOTIMPL}, ...]`
  * 02 - read sensor `[02,SensorID,Channel]`
  * 03 - read event `[03,EventID]`
  * 04 - set actuator `[04,ActuatorID,Channel,Value]`
  * 05 - set global time `[05,YY,YY,MM,DD,hh,mm,ss,tt]`

## Sensors: ##
  * 00 - System status `[00-OK, 01-BTLDR, ...]`
  * 01 - ADC input
  * 02 - PWM channel
  * 03 - Temp sensor

## Actuators: ##
  * 80 - System operations
  * 81 - ADC options
  * 82 - PWM channel `{Value}`
  * 83 - Temp sensor options `{00 - Rescan, 01 - Scan period }`
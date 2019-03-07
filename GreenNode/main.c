#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>

#include "timer.h"
#include "sio.h"
#include "adc.h"
#include "config.h"
#include "twi_slave.h"

static uchar mcucsr;

void io_init()
{
    mcucsr = MCUSR;

    // Input/Output Ports initialization
    // Port B initialization
    // Func7=In Func6=In Func5=In Func4=In Func3=In Func2=Out Func1=Out Func0=In
    // State7=T State6=T State5=T State4=T State3=T State2=0 State1=0 State0=T
    PORTB = 0x38;
    DDRB = 0x07;

    // Port C initialization
    // Func6=In Func5=In Func4=In Func3=In Func2=In Func1=Out Func0=In
    // State6=T State5=T State4=T State3=T State2=T State1=0 State0=T
    PORTC = 0x08;
    DDRC = 0x00;

    // Port D initialization
    // Func7=In Func6=In Func5=In Func4=In Func3=Out Func2=Out Func1=Out Func0=In
    // State7=T State6=T State5=T State4=T State3=1 State2=0 State1=1 State0=P
    PORTD = 0x03;
    DDRD = 0xE6;

    // Timer/Counter 0 initialization
    // Clock source: System Clock
    // Clock value: Timer 0 Stopped
    // Mode: Normal top=FFh
    // OC0A output: Disconnected
    // OC0B output: Disconnected
    TCCR0A = 0x00;
    TCCR0B = 0x00;
    TCNT0 = 0x00;
    OCR0A = 0x00;
    OCR0B = 0x00;
    // Timer/Counter 0 Interrupt(s) initialization
    TIMSK0 = 0x00;

    // Timer/Counter 1 initialization
    // Clock source: System Clock
    // Clock value: Timer 1 Stopped
    // Mode: Normal top=FFFFh
    // OC1A output: Discon.
    // OC1B output: Discon.
    // Noise Canceler: Off
    // Input Capture on Falling Edge
    // Timer 1 Overflow Interrupt: Off
    // Input Capture Interrupt: Off
    // Compare A Match Interrupt: Off
    // Compare B Match Interrupt: Off
    TCCR1A = 0x00;
    TCCR1B = 0x00;
    TCNT1H = 0x00;
    TCNT1L = 0x00;
    ICR1H = 0x00;
    ICR1L = 0x00;
    OCR1AH = 0x00;
    OCR1AL = 0x00;
    OCR1BH = 0x00;
    OCR1BL = 0x00;
    // Timer/Counter 1 Interrupt(s) initialization
    TIMSK1 = 0x00;

    // Timer/Counter 2 initialization
    // Clock source: System Clock
    // Clock value: 11.719 kHz
    // Mode: CTC top=OCR2A
    // OC2A output: Disconnected
    // OC2B output: Disconnected
    ASSR = 0x00;
    TCCR2A = 0x02;
    TCCR2B = 0x07;
    TCNT2 = 0x00;
    OCR2A = 0x75;
    OCR2B = 0x00;
    // Timer/Counter 2 Interrupt(s) initialization
    TIMSK2 = 0x02;

    // External Interrupt(s) initialization
    // INT0: Off
    // INT1: Off
    // Interrupt on any change on pins PCINT0-7: Off
    // Interrupt on any change on pins PCINT8-14: Off
    // Interrupt on any change on pins PCINT16-23: Off
    EICRA = 0x00;
    EIMSK = 0x00;
    PCICR = 0x00;

    // Analog Comparator initialization
    // Analog Comparator: Off
    // Analog Comparator Input Capture by Timer/Counter 1: Off
    ACSR = 0x80;
    ADCSRB = 0x00;

    cfg_init();
    adc_init();
    sio_init();

    // Watchdog Timer initialization
    // Watchdog Timer Prescaler: OSC/1024k
  	wdt_reset();
    WDTCSR = 0x39;
    asm (" NOP" );
    WDTCSR = 0x29;

    set_sleep_mode(SLEEP_MODE_IDLE);
    wdt_enable(WDTO_2S);

    // Global enable interrupts
    sei();
}

#include <stdio.h>

static int uart_putchar(char c, FILE *stream)
{
  return sio_putchar(c);
}

/* ADC Channels:
 00  ADC0 - 3.3V PiZ
 01  ADC1 - 5V PiZ
 02  ADC2 - 3.3V 1W
 03  ADC3
 04  ADC4
 05  ADC5
 06  ADC6 - 12V In
 07  ADC7 - 12V PS
 0E  1.30V (VBG)
 0F  0V (GND)
*/

enum
{
    ST_BOOT = 0,
    ST_POWER_ON,
    ST_ACTIVE,
    ST_TIMEOUT,
    ST_POWER_OFF
};

enum
{
    CTRL_RELAY_1 = 0x01,
    CTRL_RELAY_2 = 0x02,
    CTRL_RELAY_3 = 0x04,
    CTRL_RELAY_4 = 0x08,
    CTRL_LED_RED = 0x100,
    CTRL_LED_GRN = 0x200
};


void led_red_on()
{
    PORTD |= 0x20;
    *(ushort*)get_reg(0x14) |= CTRL_LED_RED;
}

void led_red_off()
{
    PORTD &= ~ 0x20;
    *(ushort*)get_reg(0x14) &= ~ CTRL_LED_RED;
}

void led_green_on()
{
    PORTD |= 0x40;
    *(ushort*)get_reg(0x14) |= CTRL_LED_GRN;
}

void led_green_off()
{
    PORTD &= ~ 0x40;
    *(ushort*)get_reg(0x14) &= ~ CTRL_LED_GRN;
}

void piz_On()
{
    printf_P(PSTR("Pi Zero -> ON\n"));
    PORTD |= 0x04;
    led_green_on();
}

void piz_Off()
{
    printf_P(PSTR("Pi Zero -> OFF\n"));
    PORTD &= ~ 0x04;
    led_green_off();
}

void apply_reg_0x14()
{
  static ushort old = 0;
  ushort diff;

  if(old != *(ushort*)get_reg(0x14))
  {
    diff = old ^ (*(ushort*)get_reg(0x14));
    old = *(ushort*)get_reg(0x14);
    
    if(diff & CTRL_RELAY_1) PORTB = (PORTB & 0x04) | ((old & CTRL_RELAY_1) ? 0x04 : 0);
    if(diff & CTRL_RELAY_2) PORTB = (PORTB & 0x02) | ((old & CTRL_RELAY_2) ? 0x02 : 0);
    if(diff & CTRL_RELAY_3) PORTB = (PORTD & 0x80) | ((old & CTRL_RELAY_3) ? 0x80 : 0);
    if(diff & CTRL_RELAY_4) PORTB = (PORTB & 0x01) | ((old & CTRL_RELAY_4) ? 0x01 : 0);
    if(diff & CTRL_LED_RED) if(old & CTRL_LED_RED) led_red_on(); else led_red_off();
    if(diff & CTRL_LED_GRN) if(old & CTRL_LED_GRN) led_green_on(); else led_green_off();
  }
}

int main()
{
    static FILE my_stdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
    long s = get_time() + 100;
    io_init();
    twi_init(0x16);

    stdout = &my_stdout;

 	for (;;)
    {
        ushort ad = 0, val;

        if(timeout_expired(s))
        {
            static long activity = -1, shutdown = -1;
            static uint8_t ch = 0, status = ST_BOOT;
            
            apply_reg_0x14();

            s = get_time() + 100;

            switch(ch)
            {
            case 0:
                ad = adc_read(ch);
                val = (ushort)(4.26913 * ad);
                *(ushort*)get_reg(ch*2) = val;
                printf_P(PSTR("3.3V Pi Zero = %d mV\n"), val);
                ch = 1;
                break;
            case 1:
                ad = adc_read(ch);
                val = (ushort)(6.46997 * ad);
                *(ushort*)get_reg(ch*2) = val;
                printf_P(PSTR("5V Pi Zero = %d mV\n"), val);
                ch = 2;
                break;
            case 2:
                ad = adc_read(ch);
                val = (ushort)(4.26295 * ad);
                *(ushort*)get_reg(ch*2) = val;
                printf_P(PSTR("3.3V 1Wire = %d mV\n"), val);
                ch = 6;
                break;
            case 6:
                ad = adc_read(ch);
                val = (ushort)(17.0763 * ad);
                *(ushort*)get_reg(ch*2) = val;
                printf_P(PSTR("12V Input = %d mV\n"), val);
                ch = 7;
                break;
            case 7:
                ad = adc_read(ch);
                val = (ushort)(17.0763 * ad);
                *(ushort*)get_reg(ch*2) = val;
                printf_P(PSTR("12V Power = %d mV\n"), val);
                ch = 15;
                break;
            case 15:
                ad = adc_read(ch);
                printf_P(PSTR("GND = %d mV\n"), ad);
                ch = 0;
                break;
            default:
                ch = 0;
                break;
            }

            //printf_P(PSTR("[%ld] LOOP: %d, %ld, %ld\n"), get_time(), status, activity, shutdown);
            switch(status)
            {
            case ST_BOOT:
            {
                if((*(ushort*)get_reg(7*2) > 12000) && (*(ushort*)get_reg(6*2) > 12000))
                {
                    activity = -1;
                    shutdown = get_time() + 15 * 100;
                    *(ushort*)get_reg(5*2) = 0;
                    status = ST_POWER_ON;
                    printf_P(PSTR("Status: ST_BOOT -> ST_POWER_ON\n"));
                }
                break;
            }
            case ST_POWER_ON:
            {
                if((*(ushort*)get_reg(7*2) < 11500) || (*(ushort*)get_reg(6*2) < 11500))
                {
                    // Wait to stabilize power
                    shutdown = get_time() + 15 * 100;
                }
                if(shutdown != -1 && timeout_expired(shutdown))
                {
                    piz_On();
                    shutdown = -1;
                    activity = get_time() + 10 * 100; // 10 sec
                    delay_t(10);
                    ad = adc_read(1);
                    val = (ushort)(6.46997 * ad);
                    *(ushort*)get_reg(1*2) = val;
                    ad = adc_read(0);
                    val = (ushort)(4.26913 * ad);
                    *(ushort*)get_reg(0*2) = val;
                }
                else if(activity != -1 && timeout_expired(activity))
                {
                    printf_P(PSTR("Overload!\n"));
                    piz_Off();
                    activity = -1;
                    shutdown = get_time() + 600L * 100; // 10 Min
                    status = ST_POWER_OFF;
                    printf_P(PSTR("Status: ST_POWER_ON -> ST_POWER_OFF\n"));
                }
                else if((shutdown == -1) && (*(ushort*)get_reg(0*2) > 3200) && (*(ushort*)get_reg(1*2) > 4900))
                {
                    activity = get_time() + 300 * 100; // 5 Min
                    status = ST_ACTIVE;
                    printf_P(PSTR("Status: ST_POWER_ON -> ST_ACTIVE\n"));
                }
                break;
            }
            case ST_ACTIVE:
            {
                static ushort prev = 0;
                if((*(ushort*)get_reg(7*2) < 11600) && (*(ushort*)get_reg(6*2) < 11600))
                {
                    printf_P(PSTR("Low power!\n"));
                    if(shutdown == -1) shutdown = get_time() + 3600L * 100; // Shutdown in 1 Hour
                }
                else if((*(ushort*)get_reg(7*2) > 12000) && (*(ushort*)get_reg(6*2) > 12000))
                {
                    // Power restored
                    shutdown = -1;
                }
                if((*(ushort*)get_reg(0*2) < 3200) || (*(ushort*)get_reg(1*2) < 4800) || (*(ushort*)get_reg(6*2) < 11300))
                {
                    printf_P(PSTR("Overload!\n"));
                    piz_Off();
                    shutdown = get_time() + 600L * 100; // 10 Min
                    status = ST_POWER_OFF;
                    printf_P(PSTR("Status: ST_ACTIVE -> ST_POWER_OFF\n"));
                }
                if(*(ushort*)get_reg(8*2) != prev) // Activity reg
                {
                    activity = get_time() + 300 * 100; // 5 Min
                    prev = *(ushort*)get_reg(8*2);
                }
                if(shutdown != -1 && timeout_expired(shutdown))
                {
                    // Power loss
                    piz_Off();
                    shutdown = -1;
                    activity = -1;
                    status = ST_BOOT;
                    printf_P(PSTR("Status: ST_ACTIVE -> ST_BOOT\n"));
                }
                if(activity != -1 && timeout_expired(activity))
                {
                    // Pi Zero hang
                    printf_P(PSTR("Pi Zero hang!\n"));
                    piz_Off();
                    activity = -1;
                    shutdown = get_time() + 60 * 100; // 1 Min
                    status = ST_POWER_OFF;
                    printf_P(PSTR("Status: ST_ACTIVE -> ST_POWER_OFF\n"));
                }
                if(*(ushort*)get_reg(5*2) != 0)
                {
                    // Shutdown timeout requested
                    activity = -1;
                    shutdown = get_time() + *(ushort*)get_reg(5*2) * 100;
                    status = ST_TIMEOUT;
                    printf_P(PSTR("Status: ST_ACTIVE -> ST_TIMEOUT\n"));
                }

                break;
            }
            case ST_TIMEOUT:
            {
                if(timeout_expired(shutdown))
                {
                    piz_Off();
                    activity = -1;
                    shutdown = get_time() + 60 * 100; // 1 Min
                    status = ST_POWER_OFF;
                    printf_P(PSTR("Status: ST_TIMEOUT -> ST_POWER_OFF\n"));
                }
                break;
            }
            case ST_POWER_OFF:
            {
                if(timeout_expired(shutdown))
                {
                    activity = -1;
                    shutdown = get_time() + 15 * 100;
                    status = ST_POWER_ON;
                    printf_P(PSTR("Status: ST_POWER_OFF -> ST_POWER_ON\n"));
                }
                break;
            }
            default:
                break;
            }
            *(ushort*)get_reg(9*2) = status;
        }
    	wdt_reset();
        sleep_mode();
    }
    return (0);
}

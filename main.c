/*
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <util/delay.h>

#include "uart.h"
#include "avr_thread.h"


void dump(const uint8_t *data, int size)
{
    int i;
    for (i = 0; i < size; i++) {
        printf("%02x ", data[i]);
    }
    printf("\n");
}


static void task1()
{
    while(true) {
        printf("%s()\n", __func__);
        //th_yield();
        th_delay(TH_SEC_TO_TICKS(1));
    }
}


static void task2()
{
    while(true) {
        printf("%s()\n", __func__);
        th_delay(TH_SEC_TO_TICKS(2));
//        th_yield();
    }
}

static void task3()
{
    while(true) {
        printf("%s(): counter reset\n", __func__);
        th_sleep();
    }
}



int main(void)
{
    cli();

    uart_init();
    stdin = &uart_stream;
    stdout = &uart_stream;
    printf("DAVR Thread Dbugger, SP=0x%"PRIx16"\n", SP);

    th_init();
    th_spawn(task1, TH_PRIORITY_LOW);
    th_spawn(task2, TH_PRIORITY_LOW);
    th_spawn(task3, TH_PRIORITY_HIGH);

    DDRA = _BV(0);
    PORTA = 0;

    sei();

    while(1) {
        //printf("main loop\n");
        th_sched();
    }

    return 0;
}

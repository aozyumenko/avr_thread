/*
 * Simple demo for avr_thread library.
 * Atmega1284p
 *
 * (C)2026 St(u)dio of Computer Games
 * Alexander Ozumenko <scg@stdio.ru>
 */

#include <stdio.h>
#include <avr/io.h>
#include <stdint.h>
#include "avr_thread.h"



// forward declarations
static int uart_puthcar(char c, FILE *stream);
static int uart_getchar();



// global variables
FILE uart_stream = FDEV_SETUP_STREAM(uart_puthcar, uart_getchar, _FDEV_SETUP_RW);
static int16_t task3_id;
static int cnt = 0;



// basic UART functions
static int uart_puthcar(char c, FILE *stream)
{
    if (c == '\n') uart_puthcar('\r', stream);
    while ((UCSR0A & _BV(UDRE0)) == 0) {}
    UDR0 = c;
    return 0;
}

static int uart_getchar()
{
    /* Wait for data to be received */
    while (!(UCSR0A & (1 << RXC0))) {}

    /* Get and return received data from buffer */
    return UDR0;
}


static void uart_init()
{
    UCSR0B |= _BV(RXEN0) | _BV(TXEN0);
    UCSR0C |= _BV(UCSZ00) | _BV(UCSZ01);

#define BAUD 115200
#include <util/setbaud.h>
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;
#if USE_2X
    UCSR0A |= _BV(U2X0);
#else
    UCSR0A &= ~_BV(U2X0);
#endif
}


// thread task functions

static void task1()
{
    while(true) {
        cnt++;
        printf("TASK1, cnt: %d\n", cnt);
        th_delay(TH_SEC_TO_TICKS(1));
    }
}


static void task2()
{
    while(true) {
        if (cnt >= 10) {
            printf("TASK2: reset cnt, wakeup THREAD 3\n");
            cnt = 0;
            th_wakeup(task3_id);
        }
        th_yield();
    }
}


static void task3()
{
    while(true) {
        th_sleep();
        printf("TASK3\n");
    }
}


// entry point
int main(void)
{
    cli();

    uart_init();
    stdin = &uart_stream;
    stdout = &uart_stream;
    printf("AVR Thread Demo\n");

    th_init();
    th_spawn(task1);
    th_spawn(task2);
    task3_id = th_spawn(task3);

    sei();

    while(1) {
        th_sched();
    }

    return 0;
}

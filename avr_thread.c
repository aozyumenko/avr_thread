/*
 * A simple library for implementing preemptive threads for AVR.
 *
 * (C)2026 St(u)dio of Computer Games
 * Alexander Ozumenko <scg@stdio.ru>
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "avr_thread.h"



/*** Global variables */

#ifdef TH_SYSTEM_CLOCK32
volatile static uint32_t m_system_clock;    /* system clock in ticks 32bit */
#else
volatile static uint16_t m_system_clock;    /* system clock in ticks 16bit */
#endif /* TH_SYSTEM_CLOCK32 */

static jmp_buf sys_env;                     /* main (base) flow context */

static th_ctx_t ths[TH_MAX];                /* threads context */
static int8_t th_last;                      /* number of registered threads */
static int8_t th_current;                   /* currently scheduled thread */



/*** Implementation */

/* system clock ISR */
ISR(TIMER2_OVF_vect, ISR_NAKED) {
    /* F_system_clock = 16Mhz / (256 * 32) = 1953.125Hz */
#ifdef TH_SYSTEM_CLOCK32
    __asm__ __volatile__ (
        "push r24            \n\t"
        "push r25            \n\t"
        "in   r24, __SREG__  \n\t"
        "push r24            \n\t"

        "lds  r24, %[ptr]    \n\t"
        "subi r24, lo8(-1)   \n\t"
        "sts  %[ptr], r24    \n\t"

        "lds  r24, %[ptr]+1  \n\t"
        "sbci r24, hi8(-1)   \n\t"
        "sts  %[ptr]+1, r24  \n\t"

        "lds  r24, %[ptr]+2  \n\t"
        "sbci r24, hlo8(-1)  \n\t"
        "sts  %[ptr]+2, r24  \n\t"

        "lds  r24, %[ptr]+3  \n\t"
        "sbci r24, hhi8(-1)  \n\t"
        "sts  %[ptr]+3, r24  \n\t"

        "pop  r24            \n\t"
        "out  __SREG__, r24  \n\t"
        "pop  r25            \n\t"
        "pop  r24            \n\t"
        "reti                \n\t"
        :
        : [ptr] "i" (&m_system_clock)
    );
#else
    __asm__ __volatile__ (
        "push r24            \n\t"
        "in   r24, __SREG__  \n\t"
        "push r24            \n\t"

        "lds  r24, %[ptr]    \n\t"
        "subi r24, lo8(-1)   \n\t"
        "sts  %[ptr], r24    \n\t"

        "lds  r24, %[ptr]+1  \n\t"
        "sbci r24, hi8(-1)   \n\t"
        "sts  %[ptr]+1, r24  \n\t"

        "pop  r24            \n\t"
        "out  __SREG__, r24  \n\t"
        "pop  r24            \n\t"
        "reti                \n\t"
        :
        : [ptr] "i" (&m_system_clock)
    );
#endif /* TH_SYSTEM_CLOCK32 */
}


/* get next thread for scheduling */
static __attribute__((noinline)) int8_t get_next_thread()
{
    int8_t th_next = th_current;
    int8_t i;

    for (i = 0; i < th_last; i++) {
        th_next = (th_next + 1) % th_last;
        if (ths[th_next].state == TH_STATE_RUN) {
            return th_next;
        } else if (ths[th_next].state == TH_STATE_DELAY) {
            if (period_expired(&ths[th_next].period)) {
                ths[th_next].state = TH_STATE_RUN;
                return th_next;
            }
        }
    }

    return -1;
}



/*** Functions for measuring periods */

/* period begins */
#ifdef TH_SYSTEM_CLOCK32
void period_start(period_t *period, uint32_t ticks)
#else
void period_start(period_t *period, uint16_t ticks)
#endif /* TH_SYSTEM_CLOCK32 */
{
    uint32_t system_clock = th_get_system_clock();
    period->start = system_clock;
    period->end = system_clock + ticks;
}


/* tests whether the period has ended */
bool period_expired(period_t *period)
{
    uint32_t system_clock = th_get_system_clock();

    if (period->end < period->start) {
        if (system_clock < period->start && system_clock >= period->end) {
            return true;
        }
    } else if (system_clock >= period->end) {
        return true;
    }
    return false;
}


#ifdef TH_SYSTEM_CLOCK32
uint32_t period_left(period_t *period)
#else
uint16_t period_left(period_t *period)
#endif /* TH_SYSTEM_CLOCK32 */
{
    if (period_expired(period)) {
        return 0;
    }
    return period->end - th_get_system_clock();
}



/*** Interface functions */

void th_init()
{
    /* initialize tasks list */
    th_last = 0;
    th_current = -1;

    /* start timer */
    m_system_clock = 0;
    TCCR2A = 0;
    TCCR2B = _BV(CS21) | _BV(CS20); /* 256 * 32 */
    TIMSK2 = _BV(TOIE2);            /* enable Overflow Interrupt */
}


int8_t th_spawn(th_proc proc)
{
    int th_id;

    if (th_last >= TH_MAX) {
        return -1;
    }
    th_id = th_last++;

    ths[th_id].proc = proc;
    ths[th_id].state = TH_STATE_RUN;

    if (setjmp(ths[th_id].env) != 0) {
        /* start the task */
        SP = (uint16_t)&ths[th_current].stack[TH_STACK_SIZE - 1];
        ths[th_current].proc();

        /* task is finished */
        ths[th_current].state = TH_STATE_STOP;
        while(1) th_yield();
    }

    return th_id;
}


/*
 * Passing control to the next thread.
 *
 * This function should be as short as possible, so that no stack space is used
 * for additional variables during its execution. For this very reason, the thread
 * scheduler has been moved to a separate function, `get_next_thread()`, which is
 * declared as "noinline".
 */
void th_sched()
{
    static int8_t th_next;

    if ((th_next = get_next_thread()) < 0) {
        return;
    }

    th_current = th_next;
    if (setjmp(sys_env) == 0) {
        longjmp(ths[th_current].env, 1);
    }
}


void th_wakeup(int th_id)
{
    if (ths[th_id].state == TH_STATE_SLEEP) {
        ths[th_id].state = TH_STATE_RUN;
    }
}


__attribute__((noinline)) void th_yield()
{
    if (setjmp(ths[th_current].env) == 0) {
        longjmp(sys_env, 1);
    }
}


#ifdef TH_SYSTEM_CLOCK32
void th_delay(uint32_t ticks)
#else
void th_delay(uint16_t ticks)
#endif /* TH_SYSTEM_CLOCK32 */
{
    ths[th_current].state = TH_STATE_DELAY;
    period_start(&ths[th_current].period, ticks);
    th_yield();
}


void th_sleep()
{
    ths[th_current].state = TH_STATE_SLEEP;
    th_yield();
}


#ifdef TH_SYSTEM_CLOCK32
uint32_t th_get_system_clock(void)
#else
uint16_t th_get_system_clock(void)
#endif /* TH_SYSTEM_CLOCK32 */
{
    uint32_t ret_clock;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        ret_clock = m_system_clock;
    }
    return ret_clock;
}

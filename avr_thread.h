/*
 * A simple library for implementing preemptive threads for AVR.
 *
 * (C)2026 St(u)dio of Computer Games
 * Alexander Ozumenko <scg@stdio.ru>
 */

#ifndef AVR_THREAD_H_INCLUDED
#define AVR_THREAD_H_INCLUDED



#include <avr/io.h>
#include <avr/interrupt.h>

#include <stdint.h>
#include <stdbool.h>
#include <setjmp.h>



/*** Defines */

#ifndef TH_MAX
#define TH_MAX                  9
#endif /* TH_MAX */

#ifndef TH_STACK_SIZE
#define TH_STACK_SIZE           512
#endif /* TH_STACK_SIZE */

#define TH_TIMER_PRESCALER      (256L * 32)

#ifdef TH_SYSTEM_CLOCK32
#define TH_MSEC_TO_TICKS(ms)    ((((uint32_t)ms) * (F_CPU / TH_TIMER_PRESCALER)) / 1000)
#define TH_SEC_TO_TICKS(s)      (((uint32_t)s) * (F_CPU / TH_TIMER_PRESCALER))
#else
#define TH_MSEC_TO_TICKS(ms)    (((ms) * (F_CPU / TH_TIMER_PRESCALER)) / 1000)
#define TH_SEC_TO_TICKS(s)      ((s) * (F_CPU / TH_TIMER_PRESCALER))
#endif /* TH_SYSTEM_CLOCK32 */

#define TH_TICKS_TO_SEC(ticks)  ((uint32_t)(ticks) * TH_TIMER_PRESCALER / F_CPU)



/*** Types declarations */

typedef struct period_s {
#ifdef TH_SYSTEM_CLOCK32
    uint32_t start;
    uint32_t end;
#else
    uint16_t start;
    uint16_t end;
#endif /* TH_SYSTEM_CLOCK32 */
} period_t;

typedef void (* volatile  th_proc)(void);

typedef enum th_state_e {
    TH_STATE_STOP = 0,
    TH_STATE_RUN,
    TH_STATE_DELAY,
    TH_STATE_SLEEP,
} th_state_t;

typedef struct {
    uint8_t stack[TH_STACK_SIZE];
    jmp_buf env;
    th_proc proc;
    th_state_t state;
    period_t period;
} th_ctx_t;



/*** Functions for measuring periods */

#ifdef TH_SYSTEM_CLOCK32
void period_start(period_t *period, uint32_t ticks);
uint32_t period_left(period_t *period);
#else
void period_start(period_t *period, uint16_t ticks);
uint16_t period_left(period_t *period);
#endif /* TH_SYSTEM_CLOCK32 */
bool period_expired(period_t *period);



/*** Interface functions */

void th_init();
int8_t th_spawn(th_proc proc);
void th_sched();
void th_wakeup(int th_id);

void th_yield();
#ifdef TH_SYSTEM_CLOCK32
void th_delay(uint32_t ticks);
#else
void th_delay(uint16_t ticks);
#endif /* TH_SYSTEM_CLOCK32 */
void th_sleep();

#ifdef TH_SYSTEM_CLOCK32
uint32_t th_get_system_clock(void);
#else
uint16_t th_get_system_clock(void);
#endif /* TH_SYSTEM_CLOCK32 */



#endif /* AVR_THREAD_H_INCLUDED */

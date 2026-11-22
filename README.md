# avr_thread

A lightweight cooperative multithreading library for **ATmega** microcontrollers.

## Overview

The thread concept greatly simplifies the development of multitasking firmware for microcontrollers. Although the ATmega architecture is somewhat dated, it remains popular among developers due to its simplicity, predictability, and ease of use. `avr_thread` brings clean, straightforward multithreading to AVR devices without the heavy overhead of a full RTOS.

### Tested Hardware
The library has been tested and verified on the following microcontrollers:
* **ATmega16**
* **ATmega8515**
* **ATmega1284**
* **ATmega2560**

---

## Architecture & Design Notes

* **Memory Requirements:** Each thread requires at least **256 bytes** of RAM for its stack and context. Make sure to choose a microcontroller with sufficient SRAM for your target number of threads.
* **Uniform Stack Allocation:** Currently, a fixed stack size is allocated equally for every thread. While this may not be the most memory-efficient approach, supporting per-thread custom stack sizes would significantly complicate the library's core logic and API.
* **Context Switching & Compiler Optimizations:** Ideally, the context-switching routine `th_sched()` would be written in pure assembly, as the C compiler tends to modify the stack frame around `setjmp()` calls to allocate local variables. To avoid stack corruption in C, `th_sched()` is kept as minimal as possible, while the actual scheduling logic is isolated in a separate `get_next_thread()` function marked with `__attribute__((noinline))` to prevent the compiler from merging them.
* **Hardware Timer:** By default, the system clock uses **TIMER2** to generate periodic overflow interrupts. However, the timer setup is isolated and can be easily adapted to another hardware timer if needed.
* **Configurable System Clock Width (16-bit vs. 32-bit):** Time tracking and delays rely on a dedicated system tick counter that can be configured as either **16-bit** or **32-bit** (via `TH_SYSTEM_CLOCK32`). The counter width determines the maximum duration a thread can sleep/delay and the longest time interval that can be measured, allowing you to balance RAM/CPU overhead against timing range based on your project's requirements.

---

## API Reference

### Thread Management & Scheduling
* `void th_init()` — Initializes the threading subsystem, resets thread tracking variables, and configures Timer2 to generate periodic system clock interrupts.
* `int8_t th_spawn(th_proc proc)` — Registers a new thread with its own stack and execution context, marking it as runnable. Returns the assigned thread ID, or `-1` if the maximum thread limit (`TH_MAX`) is reached.
* `void th_sched()` — Runs one scheduling cycle by saving the main system context and switching execution control to the next runnable thread.
* `void th_yield()` — Voluntarily suspends the currently running thread by saving its context and returning control back to the scheduler.
* `void th_delay(uint16_t/uint32_t ticks)` — Suspends the current thread (`TH_STATE_DELAY`) for a specified number of system clock ticks and yields execution.
* `void th_sleep()` — Puts the current thread into an indefinite sleep state (`TH_STATE_SLEEP`) until another thread explicitly wakes it up.
* `void th_wakeup(int th_id)` — Transitions a sleeping thread (`TH_STATE_SLEEP`) back into the runnable state (`TH_STATE_RUN`).
* `uint16_t/uint32_t th_get_system_clock(void)` — Atomically reads and returns the current value of the system tick counter with interrupts disabled to prevent data tearing.

### Time & Period Measurement
* `void period_start(period_t *period, uint16_t/uint32_t ticks)` — Starts a new time measurement period by recording the current system clock and calculating the target expiration tick.
* `bool period_expired(period_t *period)` — Checks whether the specified time period has elapsed, safely handling system clock counter overflows.
* `uint16_t/uint32_t period_left(period_t *period)` — Returns the number of ticks remaining until the given period expires, or `0` if it has already ended.

### Internal & Interrupt Functions
* `ISR(TIMER2_OVF_vect, ISR_NAKED)` — Timer2 overflow interrupt service routine that increments the global system tick counter (`m_system_clock`) using minimal inline assembly to preserve CPU registers and status flags.
* `static int8_t get_next_thread()` — Selects the next runnable thread in round-robin order, automatically waking up delayed threads whose timeout periods have expired.

---

## Example

A minimal usage example demonstrating thread creation, scheduling, and delays is provided in [`main.c`](main.c).

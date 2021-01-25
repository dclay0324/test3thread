/* 
 * file: testpreempt.c 
 */
#include <8051.h>
#include "preemptive.h"

/* 
 * @@@ [2pt] 
 * declare your global variables here, for the shared buffer 
 * between the producer and consumer.  
 * Hint: you may want to manually designate the location for the 
 * variable.  you can use
 *        __data __at (0x30) type var; 
 * to declare a variable var of the type
 */

__data __at (0x2A) char buffer[3];
__data __at (0x2D) char rear;
__data __at (0x2E) char front;
__data __at (0x3B) char Token;
__data __at (0x3C) char mutex;
__data __at (0x3D) char empty;
__data __at (0x3E) char full;
__data __at (0x3F) char Token2;

#define MACRO3(x) x##$
#define MACRO2(x) MACRO3(x)
#define MACRO MACRO2(__COUNTER__)
#define SemaphoreWait(s) SemaphoreWaitBody(s, MACRO)

/* [8 pts] for this function
 * the producer in this test program generates one characters at a
 * time from 'A' to 'Z' and starts from 'A' again. The shared buffer
 * must be empty in order for the Producer to write.
 */
void Producer1(void)
{
    Token = 'A';
    while (1) {
        SemaphoreWait(empty);
        SemaphoreWait(mutex);
        __critical{
            buffer[rear] = Token;
            rear = (rear + 1) % 3;
            Token = (Token == 'Z') ? 'A' : Token + 1;
        }
        SemaphoreSignal(mutex);
        SemaphoreSignal(full);
    }
}

void Producer2(void)
{
    Token2 = '0';
    while (1) {
        SemaphoreWait(empty);
        SemaphoreWait(mutex);
        __critical{
            buffer[rear] = Token2;
            rear = (rear + 1) % 3;
            Token2 = (Token2 == '9') ? '0' : Token2 + 1;
        }
        SemaphoreSignal(mutex);
        SemaphoreSignal(full);
    }
}

/* [10 pts for this function]
 * the consumer in this test program gets the next item from
 * the queue and consume it and writes it to the serial port.
 * The Consumer also does not return.
 */
void Consumer(void)
{
    /* @@@ [2 pt] initialize Tx for polling */
    EA = 0;       // disable interrupt
    TMOD |= 0x20; // to send
    TH1 = -6;     // 4800 baud
    SCON = 0x50;  // 8-bit 1 stop REN
    TR1 = 1;      // start timer 1
    EA = 1;       // enable interrupt
    while (1)
    {
        SemaphoreWait(full);
        SemaphoreWait(mutex);
        __critical{
            SBUF = buffer[front];
            front = (front + 1) % 3;
        }
        SemaphoreSignal(mutex);
        SemaphoreSignal(empty);

        while (!TI){}; // TI == 1 when ready for next byte
        TI = 0;
    }
}

/* [5 pts for this function]
 * main() is started by the thread bootstrapper as thread-0.
 * It can create more thread(s) as needed:
 * one thread can acts as producer and another as consumer.
 */
void main(void)
{
    SemaphoreCreate(mutex, 1);
    SemaphoreCreate(empty, 3);
    SemaphoreCreate(full, 0);
    rear = 0;
    front = 0;
    ThreadCreate(Producer2);
    ThreadCreate(Producer1);
    Consumer();
}

void _sdcc_gsinit_startup(void)
{
    __asm 
        ljmp _Bootstrap
    __endasm;
}

void _mcs51_genRAMCLEAR(void) {}
void _mcs51_genXINIT(void) {}
void _mcs51_genXRAMCLEAR(void) {}

void timer0_ISR(void) __interrupt(1) {
        __asm
            ljmp _myTimer0Handler
        __endasm;
}

/*
 * file: preemptive.h
 *
 * this is the include file for the PREEMPTIVE multithreading
 * package.  It is to be compiled by SDCC and targets the EdSim51 as
 * the target architecture.
 *
 * CS 3423 Fall 2018
 */



#ifndef __PREEMPTIVE_H__
#define __PREEMPTIVE_H__

#define MAXTHREADS 4  /* not including the scheduler */
/* the scheduler does not take up a thread of its own */

#define CNAME(s) _ ## s

#define SemaphoreCreate(s, n) \
    s = n\

#define SemaphoreWaitBody(s, label)\
    { __asm \
        label:\
        MOV ACC, CNAME(s)\
        JZ label\
        JB ACC.7, label\
        dec CNAME(s)\
      __endasm; }

#define SemaphoreSignal(s) \
    { __asm \
        INC CNAME(s)\
      __endasm;}

typedef char ThreadID;
typedef void (*FunctionPtr)(void);

ThreadID ThreadCreate(FunctionPtr);
void ThreadYield(void);
void ThreadExit(void);
void myTimer0Handler();

#endif // __PREEMPTIVE_H__
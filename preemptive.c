#include <8051.h>

#include "preemptive.h"

/*
 * @@@ [2 pts] declare the static globals here using 
 *        __data __at (address) type name; syntax
 * manually allocate the addresses of these variables, for
 * - saved stack pointers (MAXTHREADS)
 * - current thread ID
 * - a bitmap for which thread ID is a valid thread; 
 *   maybe also a count, but strictly speaking not necessary
 * - plus any temporaries that you need.
 */
__data __at (0x30) char saved_sp[MAXTHREADS];
__data __at (0x34) char bitmap;
__data __at (0x35) char cur_threadID;
__data __at (0x36) char i;
__data __at (0x37) char sp_temp;
__data __at (0x38) char new_thread;
__data __at (0x39) char pre_threadID;

/*
 * @@@ [8 pts]
 * define a macro for saving the context of the current thread by
 * 1) push ACC, B register, Data pointer registers (DPL, DPH), PSW
 * 2) save SP into the saved Stack Pointers array
 *   as indexed by the current thread ID.
 * Note that 1) should be written in assembly, 
 *     while 2) can be written in either assembly or C
 */
#define SAVESTATE \
        { __asm \
            PUSH ACC\
            PUSH B\
            PUSH DPL\
            PUSH DPH\
            PUSH PSW\
         __endasm; \
         saved_sp[cur_threadID] = SP;\
        }

/*
 * @@@ [8 pts]
 * define a macro for restoring the context of the current thread by
 * essentially doing the reverse of SAVESTATE:
 * 1) assign SP to the saved SP from the saved stack pointer array
 * 2) pop the registers PSW, data pointer registers, B reg, and ACC
 * Again, popping must be done in assembly but restoring SP can be
 * done in either C or assembly.
 */
#define RESTORESTATE \
         { SP = saved_sp[cur_threadID];\
          __asm \
            POP PSW\
            POP DPH\
            POP DPL\
            POP B\
            POP ACC\
          __endasm; \
         }


 /* 
  * we declare main() as an extern so we can reference its symbol
  * when creating a thread for it.
  */

extern void main(void);

/*
 * Bootstrap is jumped to by the startup code to make the thread for
 * main, and restore its context so the thread can run.
 */

void Bootstrap(void) {
      bitmap = 0;
      TMOD = 0;   // timer 0 mode 0
      IE = 0x82;  // enable timer 0 interrupt; keep consumer polling
                  // EA  -  ET2  ES  ET1  EX1  ET0  EX0
      TR0 = 1;    // set bit TR0 to start running timer 0
      pre_threadID = 2;
      cur_threadID = ThreadCreate( main );
      RESTORESTATE;
}

/*
 * ThreadCreate() creates a thread data structure so it is ready
 * to be restored (context switched in).
 * The function pointer itself should take no argument and should
 * return no argument.
 */
ThreadID ThreadCreate(FunctionPtr fp) {
         EA = 0;
         if (bitmap == 15) // bitmap == 1111
            return -1;
         
         if (!(bitmap & 1)){ // thread 0 is not used
            bitmap |= 1;     // set thread 0 bitmap to 1
            new_thread = 0;
         } else if (!(bitmap & 2)){
            bitmap |= 2;
            new_thread = 1;
         } else if (!(bitmap & 4)){
            bitmap |= 4;
            new_thread = 2;
         } else if (!(bitmap & 8)){
            bitmap |= 8;
            new_thread = 3;
         }

         sp_temp = SP;
         SP = 0x3F + 16 * new_thread;

         __asm
            // d
            PUSH DPL
            PUSH DPH
            // e
            MOV B, #0
            MOV ACC, B
            MOV DPH, B
            MOV DPL, B
            PUSH ACC
            PUSH B
            PUSH DPL
            PUSH DPH
         __endasm;

         // f
         PSW = new_thread << 3;
         __asm
            PUSH PSW
         __endasm;

         // g
         saved_sp[new_thread] = SP;
         // h
         SP = sp_temp;
         // i
         EA = 1;
         return new_thread;

        /*
         * @@@ [5 pts]
         *     otherwise, find a thread ID that is not in use,
         *     and grab it. (can check the bit bitmap for threads),
         *
         * @@@ [18 pts] below
         * a. update the bit bitmap 
             (and increment thread count, if you use a thread count, 
              but it is optional)
           b. calculate the starting stack location for new thread
           c. save the current SP in a temporary
              set SP to the starting location for the new thread
           d. push the return address fp (2-byte parameter to
              ThreadCreate) onto stack so it can be the return
              address to resume the thread. Note that in SDCC
              convention, 2-byte ptr is passed in DPTR.  but
              push instruction can only push it as two separate
              registers, DPL and DPH.
           e. we want to initialize the registers to 0, so we
              assign a register to 0 and push it four times
              for ACC, B, DPL, DPH.  Note: push #0 will not work
              because push takes only direct address as its operand,
              but it does not take an immediate (literal) operand.
           f. finally, we need to push PSW (processor status word)
              register, which consist of bits
               CY AC F0 RS1 RS0 OV UD P
              all bits can be initialized to zero, except <RS1:RS0>
              which selects the register bank.  
              Thread 0 uses bank 0, Thread 1 uses bank 1, etc.
              Setting the bits to 00B, 01B, 10B, 11B will select 
              the register bank so no need to push/pop registers
              R0-R7.  So, set PSW to 
              00000000B for thread 0, 00001000B for thread 1,
              00010000B for thread 2, 00011000B for thread 3.
           g. write the current stack pointer to the saved stack
              pointer array for this newly created thread ID
           h. set SP to the saved SP in step c.
           i. finally, return the newly created thread ID.
         */
}



/*
 * this is called by a running thread to yield control to another
 * thread.  ThreadYield() saves the context of the current
 * running thread, picks another thread (and set the current thread
 * ID to it), if any, and then restores its state.
 */

void ThreadYield(void) {
      EA = 0;
      SAVESTATE;
      do {
         cur_threadID = (cur_threadID < 3)? cur_threadID+1: 0;

         // if thread is using, swap to it, break the while loop
         if( bitmap & (1<<cur_threadID) ){
            break;
         }     
      } while (1);
      RESTORESTATE;
      EA = 1;
}


/*
 * ThreadExit() is called by the thread's own code to termiate
 * itself.  It will never return; instead, it switches context
 * to another thread.
 */
void ThreadExit(void) {
        EA = 0;
        // exit the current thread
        if (cur_threadID == 0) bitmap -= 1;
        else if (cur_threadID == 1) bitmap -= 2;
        else if (cur_threadID == 2) bitmap -= 4;
        else if (cur_threadID == 3) bitmap -= 8;

        // change to the next valid thread
        for (i = 0; i < 4; i++){
           if (bitmap & (1<<i)){
              cur_threadID = i;
              break;
           }
        }
        RESTORESTATE;
        EA = 1;
}

void myTimer0Handler(void) {
       // do not do __critial
       EA = 0;
       SAVESTATE;
       do {
          if (cur_threadID != 0){
             cur_threadID = 0;
          }
          else{
             if (pre_threadID == 1){
                cur_threadID = 2;
                pre_threadID = 2;
             }
             else if (pre_threadID == 2){
                cur_threadID = 1;
                pre_threadID = 1;
             }
          }
          if ( bitmap & (1<<cur_threadID) ){
                  break;
          }
       } while (1);
       RESTORESTATE;
       EA = 1;
       __asm 
         RETI
       __endasm;
}
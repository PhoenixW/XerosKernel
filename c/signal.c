/* signal.c - support for signal handling
   This file is not used until Assignment 3
 */

#include <xeroskernel.h>
#include <xeroslib.h>
#include "kbd.h"

int getHighestSignalNumber(int sig_mask);

void sigtramp(void (*handler)(void *), void *cntx){
	handler(cntx);
	syssigreturn(cntx);
}

int signal(pcb* to_signal, int signalNumber){
	//if idle process, or to_kill DNE or state stopped, return -514
	if(to_signal->pid == 0 || to_signal == NULL || to_signal->state == STATE_STOPPED){
        return -514;
        //if signal number out of range
    } else if(signalNumber < 0 || signalNumber > 31){
        return -583;
        if(to_signal->handlers[signalNumber] != NULL){
            int mask = 1 << signalNumber;
            to_signal->sig_mask |= mask;
            // If the target p is already on readyQ, don't worry,
            // if it's not on readyQ, it is blocked, but should be unblocked to receive signal
            if(to_signal->state != STATE_READY){
                if(to_signal->state == STATE_SLEEP){
            		to_signal->ret =  to_signal->sleepTicks * PREEMPT_INTERVAL;
            	}else if(to_signal->state == STATE_KBD_BLOCKED){
					to_signal->ret =  kbdSignalReturn();
				}else{
            		to_signal->ret = -666;
				}
				removeFromQueue(to_signal->currentQueue, to_signal);
				ready(to_signal);
            }
        }
	}
	return 0;
}

void sigreturn(void* context, pcb* p) {
	int* old_sp = (int *)(context);
	int prev_signal = *(old_sp - 1);
	p->esp = (unsigned long *) old_sp;
	unsigned int mask = ~(1 << p->currentSignalLevel);
	p->sig_mask &= mask;
	p->currentSignalLevel = prev_signal;

	//check again for lower priority signals that were not previously on stack
	sigsetup(p);
}

void sigsetup(pcb* p){
	//signalNum should never be 31 becase we check for it in syskill
	//get highest signalNumber in mask
	int signalNum = getHighestSignalNumber(p->sig_mask);
	//if highest signalNum in bitmask is less than or equal
	//to current signal we are processing, ignore it
	if(signalNum <= p->currentSignalLevel){
		return;
	}
	signalFrame * sf = (signalFrame *)((unsigned char*)p->esp - sizeof(signalFrame));
	sf->returnAddress = NULL;
	sf->handler = p->handlers[signalNum];
	sf->context = p->esp;
	sf->signalNumber = p->currentSignalLevel;

	context_frame       *cf;
	cf = (context_frame *)((unsigned char *)sf - sizeof(context_frame));
	memset(cf, 0xA5, sizeof( context_frame ));
    cf->iret_cs = getCS();
    cf->iret_eip = (unsigned int)&sigtramp;
    cf->eflags = STARTING_EFLAGS;
    cf->esp = (int)(cf + 1);
    cf->ebp = cf->esp;

	p->currentSignalLevel = signalNum;
	p->esp = (unsigned long*) cf;
}

int getHighestSignalNumber(int sig_mask){
	unsigned int mask = 1 << 31;
	for(int i = 31; i >= 0; i--) {
		if((mask & sig_mask) == mask){
			return i;
		}
		mask >>= 1;
	}
	return -1;
}

void ignore(void){
    return;
}

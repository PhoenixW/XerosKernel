/* create.c : create a process
 */

#include <xeroskernel.h>
#include <xeroslib.h>

// pcb     pcbTable[MAX_PROC];

/* make sure interrupts are armed later on in the kernel development  */

// PIDs start at 1 except for idle process which will always have PID 0
static PID_t      nextpid = 0;

extern struct queue stoppedQueue;

int create( funcptr fp, size_t stackSize ) {
/***********************************************/

    context_frame       *cf;
    pcb                 *p = pop(&stoppedQueue);

    /* If the PID becomes 0 it  has wrapped.
     * This means that the next PID we handout could be
     * in use. To find such a free number we have to propose a
     * new PID and then scan to see if it is in the table. If it
     * is then we have to try again.
     */
    //commented out to allow idle process to have PID 0
    // if (nextpid == 0)
    //   return CREATE_FAILURE;

    // If the stack is too small make it larger
    if( stackSize < PROC_STACK ) {
        stackSize = PROC_STACK;
    }

    if( !p ) {
        return CREATE_FAILURE;
    }


    cf = kmalloc( stackSize );
    if( !cf ) {
        return CREATE_FAILURE;
    }
    p->startOfMemSpace = (unsigned long)cf;
    p->stackSize = stackSize;
    cf = (context_frame *)((unsigned char *)cf + stackSize - SAFETY_MARGIN);

    // set the return address of the process to be sysstop, to cleanup the process when it ends.
    unsigned long* ra = ((unsigned long*)cf) - 1;
    ra[0] = (unsigned long)sysstop;
    cf = (context_frame*)ra;

    cf--;

    memset(cf, 0xA5, sizeof( context_frame ));

    cf->iret_cs = getCS();
    cf->iret_eip = (unsigned int)fp;
    cf->eflags = STARTING_EFLAGS;

    cf->esp = (int)(cf + 1);
    cf->ebp = cf->esp;
    p->esp = (unsigned long*)cf;
    p->state = STATE_READY;
    p->priority = 3;
    p->pid = nextpid++;
    p->senders = (struct queue){NULL, NULL};
    p->receivers = (struct queue){NULL, NULL};
    for(int i = 0; i < 31; i++){
        p->handlers[i] = NULL;
    }
    // p->handlers[31] = &sysstop;
    p->sig_mask = 0;
    p->currentSignalLevel = -1;

    for(int i = 0; i < FDT_SIZE; i++){
        p->fdt[i].dvnum = -1;
    }

    ready( p );
    return p->pid;
}

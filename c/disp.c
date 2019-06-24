/* disp.c : dispatcher
 */

#include <xeroskernel.h>
#include <xeroslib.h>
#include <stdarg.h>
#include "i386.h"
#include "kbd.h"

/* -------- Declare pcb and queue types, and declare the required queues. --------- */
extern  char    *maxaddr;   /* max memory address (set in i386.c)   */

struct queue priorityQueue0;
struct queue priorityQueue1;
struct queue priorityQueue2;
struct queue priorityQueue3;
struct queue stoppedQueue;
struct queue receiveAnyQueue;
struct queue kbdQueue;
int currPrioQue = 0;

struct queue sleepQueue;

static pcb pcbTable[MAX_PROC];
devsw devTable[2];

/* ------------ Implement a queue of pcbs. -------------------------- */

/*
* Initialize an empty queue.
*/
void initQueue(struct queue* q){
    q->front = NULL;
    q->rear = NULL;
}

// 0 indicates p wasn't found in queue
// 1 indicates that p was removed successfully
int removeFromQueue(struct queue* q, pcb* p) {
    if(p == NULL){
        return 0;
    }
    //pcb to remove is front of que
    if(p->pid == q->front->pid) {
        //only item in list
        if(q->front->next == NULL){
            q->front = NULL;
            q->rear = NULL;
        } else {
            q->front = q->front->next;
        }
        return 1;
    }

    pcb* currPCB = q->front;
    pcb* prevPCB = q->front;
    while(currPCB != NULL){
        if(currPCB->pid == p->pid){
            prevPCB->next = currPCB->next;
            //pcb to remove is rear
            if(p->pid == q->rear->pid){
                q->rear = prevPCB;
            }
            return 1;
        }
        prevPCB = currPCB;
        currPCB = currPCB->next;
    }
    return 0;
}

/*
* push a new element into the (rear of the) queue.
*/
void push(struct queue* q, pcb* newElem){
    if(q->rear == NULL){
        newElem->next=NULL;
        q->front = newElem;
        q->rear = newElem;
    }else{
        q->rear->next = newElem;
        newElem->next = NULL;
        q->rear = newElem;
    }
    newElem->currentQueue = q;
}

/*
* pop an element off the front of the queue.
*/
pcb* pop(struct queue* q){
    if(q->front == NULL){
        return NULL;
    }else if (q->front == q->rear){
        pcb* front = q->front;
        q->front = NULL;
        q->rear = NULL;
        front->currentQueue = NULL;
        return front;
    }else{
        pcb* front = q->front;
        q->front = front->next;
        front->currentQueue = NULL;
        return front;
    }
    return NULL;
}

pcb* getPCBByPID(PID_t pid){
    for(int i = 0; i < MAX_PROC; i++) {
        if(pcbTable[i].pid == pid){
            return &(pcbTable[i]);
        }
    }
    return NULL;
}

/*
* Return the number of non-stopped processes, including the null process.
*/
int getNumProcesses(void){
    int count = 0;
    for(int i = 0; i < MAX_PROC; i++){
        if(pcbTable[i].state != STATE_STOPPED){
            count++;
        }
    }
    return count;
}

/*
* Search a queue for a desired process by specifying the desired process' PID.
* Return the process if found in queue, NULL otherwise.
*/
pcb* findInQ(struct queue* q, PID_t pid){
    pcb* p = q->front;
    while(p != NULL){
        if(p->pid == pid){
            return p;
        }
        p = p->next;
    }
    return NULL;
}

//returns a count of the # of pcb's in a que;
int test_countQueue(struct queue* q){
    int i;
    pcb* p = q->front;
    if(p == NULL){
        return 0;
    } else {
        i = 1;
    }
    while(p->next != NULL){
        p = p->next;
        i++;
    }
    return i;
}

void test_printPIDsinQueue(struct queue* q){
    pcb* p = q->front;
    while(p != NULL){
        kprintf("p->pid: %d\n", p->pid);
        p = p->next;
    }
}

struct queue* getQueueByPriority(int prio){
    switch(prio){
        case(0):
            return &(priorityQueue0);
        case(1):
            return &(priorityQueue1);
        case(2):
            return &(priorityQueue2);
        case(3):
            return &priorityQueue3;
        default:
            return NULL;
    }
}

/* -------------------------------------------------------------------- */

/*
* Initialize the dispatcher by creating an array of pcbs (pcbTable) and initializing
* all the queues.
*/
void dispatchinit( void ){
    initQueue(&stoppedQueue);
    initQueue(&receiveAnyQueue);
    initQueue(&priorityQueue0);
    initQueue(&priorityQueue1);
    initQueue(&priorityQueue2);
    initQueue(&priorityQueue3);
    initQueue(&sleepQueue);

    memset(pcbTable, 0, sizeof( pcb ) * MAX_PROC);

    /*Explicilty set states to STOPPED,
    all processes to stopped queue,
    and priority to 3*/
    pcb* currTable;
    for(int i = 0; i <= 32; i++){
        currTable = pcbTable + i;
        currTable->state = STATE_STOPPED;
        currTable->priority = 3;
        push(&stoppedQueue, currTable);
    }
}

/*
* This function removes the next process from the ready queue and returns an index
* or a pointer to its process control block
*/
pcb* next(void){
    // not added to running queue b/c only one process runs at a time
    struct queue* nextQueue = getQueueByPriority(currPrioQue);
    pcb* nextP;

    while(nextQueue->front == NULL){
        if(currPrioQue == 3){
            kprintf("out of processes!\n");
            break;
        } else {
            currPrioQue = currPrioQue + 1;
            // kprintf("decrementing currPrioQueue: %d\n");
            nextQueue = getQueueByPriority(currPrioQue);
        }
    }

    nextP = pop(nextQueue);
    if(currPrioQue == 3 && nextQueue->front != NULL && nextP->pid == 0){
        push(nextQueue, nextP);
        nextP = pop(nextQueue);
    }
    if(nextP->sig_mask != 0){
    	sigsetup(nextP);
    }
    return nextP;
}

/*
* This function takes an index or pointer to a process control block and adds it
* to the ready queue.
*/
void ready(pcb* ptr){
    ptr->state = STATE_READY;
    if(ptr->priority < 0 || ptr->priority > 3){
        kprintf("Invalid process priority\n");
    }else {
        if(ptr->priority < currPrioQue){
            //if we add a process to a lower priority que, we update this tracker
            currPrioQue = ptr->priority;
        }
        push(getQueueByPriority(ptr->priority), ptr);
    }
}

/*
* stop the process by adding it to the stopped queue.
*/
//process must be removed from queue before calling cleanup
void cleanup(pcb* ptr){

    // Cleanup IPC stuff - this process can't be in anyone's senders/receivers Q or the receiveAnyQueue, otherwise it would be blocked
    pcb* curr = pop(&(ptr->senders));
    while( curr != NULL ){
        curr->ret = -1;
        ready(curr);
        curr = pop(&(ptr->senders));
    }

    curr = pop(&(ptr->receivers));
    while( curr != NULL ){
        curr->ret = -1;
        ready(curr);
        curr = pop(&(ptr->receivers));
    }

    curr = pop(&(ptr->waiters));
    while( curr != NULL ){
        curr->ret = 0;
        ready(curr);
        curr = pop(&(ptr->waiters));
    }

    /* There is no protection against all remaining processes being in a receiveAny state.
    If this happens, the system will hang. */

    // After this process terminates, there may only be one real process left - there should always be at least the idle process
    if(getNumProcesses() <= 3){
        /* if the only other real process is blocked on a receiveAny() call, it will soon be the only non-null process
        in the system, so it should return -10 and be added to the ready queue. */
        if(receiveAnyQueue.front){
            pcb* receiveAnyProcess = pop(&receiveAnyQueue);
            receiveAnyProcess->ret = -10;
            ready(receiveAnyProcess);
        }
    }

    // Now stop the actual process
    ptr->state = STATE_STOPPED;
    ptr->priority = 3;
    kfree((void*)ptr->startOfMemSpace);
    push(&stoppedQueue, ptr);
}

int removeFromQueueAndCleanUp(pcb* p){
	if(removeFromQueue(p->currentQueue, p)){
		cleanup(p);
        return 1;
	}
    return 0;
}

/*
* This function is an infinite loop, and is the last thing to be run in initproc().
* Dispatch() processes system calls and schedules processes.
*/
void     dispatch( void ) {
/********************************/

    pcb         *p;
    int         r;
    funcptr     fp;
    int         stack;
    va_list     ap;

    for( p = next(); p; ) {
      r = contextswitch( p );
      switch( r ) {
          case( SYS_CREATE ):
            ap = (va_list)p->args;
            fp = (funcptr)(va_arg( ap, int ) );
            stack = va_arg( ap, int );
    	    p->ret = create( fp, stack );
            break;

          case( SYS_YIELD ):
            ready( p );
            p = next();
            break;

          case( SYS_STOP ):
            cleanup(p);
            p = next();
            break;

          case( SYS_GETPID ):
            p->ret = p->pid;
            break;

          case( SYS_PUTS ):
            ap = (va_list)p->args;
            char* str = va_arg(ap, char*);
            kprintf(str);
            break;

          case( SYS_KILL ):
            ap = (va_list)p->args;
            PID_t pid = va_arg( ap, PID_t );
            int signalNumber = va_arg( ap, int );
            if(signalNumber == 31 || pid == 0){
            	pcb* p_toKill = getPCBByPID(pid);
                if(p_toKill == NULL){
                    p->ret = -514;
                    break;
                }
            	if(p_toKill->pid == p->pid){
            		cleanup(p_toKill);
                    p->ret = 0;
            		p = next();
                    break;
            	} else {
                    if(removeFromQueueAndCleanUp(p_toKill)){
                        p->ret = 0;
                    }else{
                        p->ret = -1;
                    }

                    break;
            	}
            } else{
                pcb* to_kill = getPCBByPID(pid);
                p->ret = signal(to_kill, signalNumber);
            }

            break;

          case( SYS_SIGHANDLER ):
            ap = (va_list)p->args;
            int signal = va_arg(ap, int);
            funcptr2 newhandler = va_arg(ap, funcptr2);
            funcptr2* oldHandler = va_arg(ap, funcptr2*);

            if(signal < 0 || signal > 30){
                p->ret = -1;
            }else if(!isValidKernelAddress((void*)newhandler)){
                p->ret = -2;
            }else if(!isValidUserAddress((void*)oldHandler)){
                p->ret = -3;
            }else{
                *oldHandler = p->handlers[signal];
                p->handlers[signal] = newhandler;
                p->ret = 0;
            }
            break;

        case( SYS_RETURN ):
        	ap = (va_list)p->args;
        	void* context = va_arg(ap, void*);
        	sigreturn(context, p);
            break;

          case( SYS_SETPRIO ):
            ap = (va_list)p->args;
            int prio = va_arg( ap, int );
            if(prio == -1){
                p->ret = p->priority;
            } else if(prio >= 0 && prio <= 3) {
                p->priority = prio;
                p->ret = prio;
            } else {
                p->ret = -1;
            }
            break;

          case( SYS_SEND ):
            ap = (va_list)p->args;
            PID_t dest_pid = va_arg(ap, PID_t);
            unsigned long send_num = va_arg(ap, unsigned long);

            pcb* dest = getPCBByPID(dest_pid);
            if(dest_pid == p->pid){
                p->ret = -3;
            }else if(!dest || dest->state == STATE_STOPPED){
                /* If it's not in blockedQueue or readyQueue, it doesn't exist.  Can't just check stoppedQueue b/c
                even if not there, it may not exist in other queues either. */
                p->ret = -2;
            }else{
                // if send blocks, let next process run
                if(!send(p, dest_pid, send_num))
                    p = next();
            }
            break;

          case( SYS_RECV ):
            ap = (va_list)p->args;
            PID_t* from_pid = va_arg(ap, PID_t*);
            unsigned int* recv_num = va_arg(ap, unsigned int*);

            /* Check that passed addresses are valid - we were told to assume the args would be local variables (therefore,
            on each process' stack). */
            if((unsigned long)from_pid < p->startOfMemSpace || (unsigned long)from_pid > p->startOfMemSpace + p->stackSize - SAFETY_MARGIN){
                p->ret = -5;
                break;
            }
            if((unsigned long)recv_num < p->startOfMemSpace || (unsigned long)recv_num > p->startOfMemSpace + p->stackSize - SAFETY_MARGIN){
                p->ret = -4;
                break;
            }

            pcb* src = getPCBByPID(*from_pid);
            if(*from_pid == p->pid){
                p->ret = -3;
            }else if(*from_pid != 0 && (!src || src->state == STATE_STOPPED)){
                /* If it's not in blockedQueue or readyQueue, it doesn't exist.  Can't just check stoppedQueue b/c
                even if not there, it may not exist in other queues either. */
                p->ret = -2;
            }else if(getNumProcesses() <= 2){
                // If there are 2 processes, then assume one of them is the null process, so this is the only real process
                p->ret = -10;
            }else{
                // if receive blocks, let next process run
                if(!recv(p, from_pid, recv_num))
                    p = next();
            }
            break;

        case( TIMER_INT ):
            tick(&sleepQueue);
            ready( p );
            p = next();
            end_of_intr();
            break;

        case ( KBD_INT ):
            kbd_isr();
            end_of_intr();
            break;

        case( SYS_SLEEP ):
            ap = (va_list)p->args;
            unsigned int sleepTimer = va_arg(ap, unsigned int);
            sleep(p, sleepTimer, &sleepQueue);
            p = next();
            break;

        case( SYS_WAIT ):
            ap = (va_list)p->args;
            PID_t target_pid = va_arg(ap, PID_t);
            pcb* target = getPCBByPID(target_pid);
            if(target == NULL || target->state == STATE_STOPPED || target_pid == 0){
                p->ret = -1;
            }else{
                push(&(target->waiters), p);
                p->state = STATE_BLOCKED;
                p = next();
            }
            break;

        case (SYS_CPUTIMES):
            ap = (va_list) p->args;
            p->ret = getCPUtimes(p, va_arg(ap, processStatuses *));
            break;

		case (SYS_DEVICEOPEN):
            ap = (va_list) p->args;
            int device_no = va_arg(ap, int);
            p->ret = di_open(device_no, p);
        	break;
        case (SYS_DEVICECLOSE):
            ap = (va_list) p->args;
            int fd = va_arg(ap, int);
            p->ret = di_close(fd, p);
        	break;
        case (SYS_DEVICEREAD):
            ap = (va_list) p->args;
            int read_fd = va_arg(ap, int);
            void* read_buff = va_arg(ap, void*);
            int read_bufflen = va_arg(ap, int);
            int val = di_read(read_fd, read_buff, read_bufflen, p);
            if(val == STATE_KBD_BLOCKED){
                push(&kbdQueue, p);
                p->state = STATE_KBD_BLOCKED;
                p = next();
            }else{
                p->ret = val;
            }
        	break;
        case (SYS_DEVICEWRITE):
            ap = (va_list) p->args;
            int write_fd = va_arg(ap, int);
            void* write_buff = va_arg(ap, void*);
            int write_bufflen = va_arg(ap, int);
            p->ret = di_write(write_fd, write_buff, write_bufflen, p);
        	break;
        case (SYS_DEVICEIOCTL):
            ap = (va_list) p->args;
            int ioctl_fd = va_arg(ap, int);
            unsigned long command = va_arg(ap, unsigned long);
            va_list other_args = va_arg(ap, va_list);
            p->ret = di_ioctl(ioctl_fd, command, other_args);
            break;
        default:
            kprintf( "Bad Sys request %d, pid = %u\n", r, p->pid );
      }
    }

    kprintf( "Out of processes: dying\n" );

    for( ;; );
}

int getCPUtimes(pcb *p, processStatuses *ps) {

  int i, currentSlot;
  currentSlot = -1;

  // Check if address is in the hole
  if (((unsigned long) ps) >= HOLESTART && ((unsigned long) ps <= HOLEEND))
    return -1;

  //Check if address of the data structure is beyone the end of main memory
  if ((((char * ) ps) + sizeof(processStatuses)) > maxaddr)
    return -2;

  // There are probably other address checks that can be done, but this is OK for now


  for (i=0; i < MAX_PROC; i++) {
    if (pcbTable[i].state != STATE_STOPPED) {
      // fill in the table entry
      currentSlot++;
      ps->pid[currentSlot] = pcbTable[i].pid;
      ps->status[currentSlot] = p->pid == pcbTable[i].pid ? STATE_RUNNING: pcbTable[i].state;
      ps->cpuTime[currentSlot] = pcbTable[i].cpuTime * MILLISECONDS_TICK;
    }
  }
  return currentSlot;
}



void deviceinit(){
    devTable[0].dvnum = 0;
    devTable[0].dvopen = &kbd_open;
    devTable[0].dvclose = &kbd_close;
    devTable[0].dvwrite = &kbd_write;
    devTable[0].dvread = &kbd_read;

    devTable[1].dvnum = 1;
    devTable[1].dvopen = &kbd_open;
    devTable[1].dvclose = &kbd_close;
    devTable[1].dvwrite = &kbd_write;
    devTable[1].dvread = &kbd_read;
}

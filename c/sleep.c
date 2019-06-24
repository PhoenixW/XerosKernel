/* sleep.c : sleep device
   This file does not need to modified until assignment 2
 */

#include <xeroskernel.h>
#include <xeroslib.h>

/* Your code goes here */
extern struct queue sleepQueue;


void insertIntosleepQ( pcb* p, struct queue* sleepQ){
	//if sleepQ is empty, insert p
	p->currentQueue = sleepQ;
	if(sleepQ->front == NULL){
		sleepQ->front = p;
		sleepQ->rear = p;
		p->next = NULL;
		return;
	}
	//p belongs at the front of the queue
	if(sleepQ->front->sleepTicks > p->sleepTicks){
		p->next = sleepQ->front;
		sleepQ->front = p;
	}

	pcb* runner = sleepQ->front;
	pcb* previous = sleepQ->front;
	while(runner != NULL){
		//if iterator has higher sleep ticks then current, insert right before
		if(runner->sleepTicks > p->sleepTicks){
			previous->next = p;
			p->next = runner;
			return;
		}
		previous = runner;
		runner = runner->next;
	}
	//if while loop runs to completion: p is new rear of queue
	sleepQ->rear->next = p;
	sleepQ->rear = p;
	p->next = NULL;
}

void decremenetsleepQ(struct queue* sleepQ){
	pcb* runner = sleepQ->front;
	while( runner != NULL ){
		runner->sleepTicks = runner->sleepTicks - 1;
		runner = runner->next;
	}
}

void wakeSleepingProcesses(struct queue* sleepQ) {
	pcb* p = sleepQ->front;

	while(p != NULL && p->sleepTicks == 0){
		sleepQ->front = p->next;
		ready(p);
		p = sleepQ->front;
	}
}

unsigned int calcTicks(unsigned int sleepTimer) {
	unsigned int ticks = (sleepTimer / (unsigned int) PREEMPT_INTERVAL);
	if(sleepTimer % PREEMPT_INTERVAL > 0) {
		ticks += 1;
	}
	return ticks;
}

void printsleepQ(struct queue* sleepQ){
	pcb* p = sleepQ->front;
	while(p != NULL){
		kprintf("p->pid %d; p->sleepTicks %d\n", p->pid, p->sleepTicks);
		p = p->next;
	}
}

void tick( struct queue *sleepQ ){
	decremenetsleepQ(sleepQ);
	wakeSleepingProcesses(sleepQ);
}

void sleep( pcb* p, unsigned int sleepTimer, struct queue* sleepQ ){
	p->sleepTicks = calcTicks(sleepTimer);
	p->state = STATE_SLEEP;
	p->currentQueue = &sleepQueue;
	insertIntosleepQ(p, sleepQ);
}

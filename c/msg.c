/* msg.c : messaging system
   This file does not need to modified until assignment 2
 */

#include <xeroskernel.h>
#include <stdarg.h>

/* Your code goes here */

extern struct queue receiveAnyQueue;


/*
* Send a message to dest_pid.  May need to block. For now, only sending a single number.
* return 1 if sender can keep running, 0 if it will block.  The syscall return value should be set in send, by
* modifying the pcb directly.
*/
int send(pcb* sender, PID_t dest_pid, unsigned long num){
    va_list ap;
    pcb* dest = getPCBByPID(dest_pid);

    if(findInQ(&(sender->senders), dest_pid)){ // If the destination process is already sending to you, return -100 and abort send to avoid deadlock
        sender->ret = -100;
    }else if(findInQ(&(sender->receivers), dest_pid)){ // check if process already waiting to receive
        ap = (va_list)dest->args;
        PID_t* from_pid = va_arg(ap, PID_t*);
        unsigned int* recv_num = va_arg(ap, unsigned int*);

        recv_num[0] = num;
        from_pid[0] = sender->pid;
        dest->ret = 0;
        // Now that data has been sent, remove the process from the receiver q
        removeFromQueue(&(sender->receivers), dest);
        ready(dest);    // since process was already waiting, it must have been blocked
        sender->ret = 0;

    }else if(findInQ(&receiveAnyQueue, dest_pid)){   // receiver is waiting on receiverAny
        ap = (va_list)dest->args;
        PID_t* from_pid = va_arg(ap, PID_t*);
        unsigned int* recv_num = va_arg(ap, unsigned int*);

        recv_num[0] = num;
        from_pid[0] = sender->pid;
        dest->ret = 0;
        removeFromQueue(&receiveAnyQueue, dest);
        ready(dest);    // since process was already waiting, it must have been blocked
        sender->ret = 0;

    }else{
        // At this point p->args still has the va_list pointing to the params, can access later b/c va_list not destroyed until after syscall returns from kernel
        push(&(dest->senders), sender);
        sender->state = STATE_BLOCKED;
        for(pcb* temp = dest->senders.front; temp != NULL; temp = temp->next){
        }
        // return 0 indicating that the sender process should not keep running (i.e. it should block)
        return 0;
    }
    return 1;
}

int recv(pcb* receiver, PID_t* src_pid, unsigned int* num){
    // if pid was invalid, dispatcher would have already returned -2

    va_list ap;
    pcb* src;
    // PID_t dest_pid;
    unsigned long src_num;

    // Receive from anyone
    if(*src_pid == 0){
        // check if there is at least one sender
        if(receiver->senders.front){
            src = pop(&(receiver->senders));
            ap = (va_list)src->args;
            // dest_pid = va_arg(ap, PID_t);
            src_num = va_arg(ap, unsigned long);

            num[0] = src_num;
            src_pid[0] = src->pid;
            src->ret = 0;
            ready(src);  // since process was already waiting, it must have been blocked
            receiver->ret = 0;
        }else{    // no senders
            push(&(receiveAnyQueue), receiver);
            receiver->state = STATE_BLOCKED;
            return 0;
        }
    }else{  // Receive from a specific process
        src = getPCBByPID(*src_pid);

        if(findInQ(&(receiver->receivers), *src_pid)){
            /* if the process you want to receive from is already trying to receive from you,
            abort the receive and return -100 to avoid deadlock. */
            receiver->ret = -100;
        }else if(findInQ(&(receiver->senders), *src_pid)){
                ap = (va_list)src->args;
                // dest_pid = va_arg(ap, PID_t);
                src_num = va_arg(ap, unsigned long);

                num[0] = src_num;
                src_pid[0] = src->pid;
                src->ret = 0;
                // Now that data has been sent, remove the process from the sender q
                removeFromQueue(&(receiver->senders), src);
                ready(src);  // since process was already waiting, it must have been blocked
                receiver->ret = 0;

        }else{
            // the sending process has not sent yet
            push(&(src->receivers), receiver);
            receiver->state = STATE_BLOCKED;
            return 0;
        }
    }
    return 1;
}

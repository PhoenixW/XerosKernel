/* syscall.c : syscalls
 */

#include <xeroskernel.h>
#include <stdarg.h>

extern void sysstop( void );

int syscall( int req, ... ) {
/**********************************/

    va_list     ap;
    int         rc;

    va_start( ap, req );

    __asm __volatile( " \
        movl %1, %%eax \n\
        movl %2, %%edx \n\
        int  %3 \n\
        movl %%eax, %0 \n\
        "
        : "=g" (rc)
        : "g" (req), "g" (ap), "i" (KERNEL_INT)
        : "%eax"
    );

    va_end( ap );

    return( rc );
}

unsigned int syscreate( funcptr fp, size_t stack ) {
/*********************************************/

    return( syscall( SYS_CREATE, fp, stack ) );
}

void sysyield( void ) {
/***************************/
  syscall( SYS_YIELD );
}

 void sysstop( void ) {
/**************************/
   syscall( SYS_STOP );
}

PID_t sysgetpid( void ) {
    return syscall(SYS_GETPID);
}

void sysputs( char *str ) {
    syscall( SYS_PUTS, str );
}

int syskill ( PID_t pid, int signalNumber ) {
    return syscall( SYS_KILL, pid, signalNumber );
}

int syssetprio( int priority ) {
    return syscall( SYS_SETPRIO, priority );
}

int syssend( unsigned int dest_pid, unsigned long num ){
    return syscall( SYS_SEND, dest_pid, num);
}

int sysrecv( unsigned int *from_pid, unsigned int * num){
    return syscall( SYS_RECV, from_pid, num);
}

unsigned int syssleep( unsigned int milliseconds ){
    return syscall( SYS_SLEEP, milliseconds);
}

int syssighandler(int signal, void (*newhandler)(void *), void (** oldHandler)(void *)){
    return syscall( SYS_SIGHANDLER, signal, newhandler, oldHandler);
}

void syssigreturn(void *old_sp){
    syscall( SYS_RETURN, old_sp);
}

int syswait(int pid){
    return syscall( SYS_WAIT, pid);
}

int sysgetcputimes(processStatuses *ps) {
  return syscall(SYS_CPUTIMES, ps);
}

int sysopen(int device_no){
    return syscall(SYS_DEVICEOPEN, device_no);
}

int sysclose(int fd){
    return syscall(SYS_DEVICECLOSE, fd);
}

int syswrite(int fd, void *buff, int bufflen){
    return syscall(SYS_DEVICEWRITE, fd, buff, bufflen);
}

int sysread(int fd, void *buff, int bufflen){
    return syscall(SYS_DEVICEREAD, fd, buff, bufflen);
}

int sysioctl(int fd, unsigned long command, ...){
    va_list args;
    va_start(args, command);
    return syscall(SYS_DEVICEIOCTL, fd, command, args);
}

/* xeroskernel.h - disable, enable, halt, restore, isodd, min, max */

#ifndef XEROSKERNEL_H
#define XEROSKERNEL_H

/* Symbolic constants used throughout Xinu */

typedef	char    Bool;        /* Boolean type                  */
typedef unsigned int size_t; /* Something that can hold the value of
                              * theoretical maximum number of bytes
                              * addressable in this architecture.
                              */
typedef unsigned int  PID_t;
typedef struct struct_pcb pcb;

/* A typedef for the signature of the function passed to syscreate */
typedef void    (*funcptr)(void);
typedef void    (*funcptr2)(void*);

#define	FALSE   0       /* Boolean constants             */
#define	TRUE    1
#define	EMPTY   (-1)    /* an illegal gpq                */
#define	NULL    0       /* Null pointer for linked lists */
#define	NULLCH '\0'     /* The null character            */

#define CREATE_FAILURE -1  /* Process creation failed     */



/* Universal return constants */

#define	OK            1         /* system call ok               */
#define	SYSERR       -1         /* system call failed           */
#define	EOF          -2         /* End-of-file (usu. from read)	*/
#define	TIMEOUT      -3         /* time out  (usu. recvtim)     */
#define	INTRMSG      -4         /* keyboard "intr" key pressed	*/
                                /*  (usu. defined as ^B)        */
#define	BLOCKERR     -5         /* non-blocking op would block  */

/* Functions defined by startup code */


void           bzero(void *base, int cnt);
void           bcopy(const void *src, void *dest, unsigned int n);
void           disable(void);
unsigned short getCS(void);
unsigned char  inb(unsigned int);
void           init8259(void);
int            kprintf(char * fmt, ...);
void           lidt(void);
void           outb(unsigned int, unsigned char);

#define TOGGLE_DEVICE_EOF 53
#define TOGGLE_DEVICE_ECHO_OFF 55
#define TOGGLE_DEVICE_ECHO_ON 56


/* Some constants involved with process creation and managment */

   /* Maximum number of processes */
#define MAX_PROC        64
   /* Kernel trap number          */
#define KERNEL_INT      80
   /* Minimum size of a stack when a process is created */
#define PROC_STACK      (4096 * 4)
// define a safety margin for processes (32 bytes)
#define SAFETY_MARGIN 32
//starting e flags for new processes
#define STARTING_EFLAGS         0x00003200
//size of file descriptor table for process
#define FDT_SIZE        4
// number of devices
#define NUM_DEVICES 2

/* Constants to track states that a process is in */
#define STATE_STOPPED   0
#define STATE_READY     1
#define STATE_BLOCKED   2
#define STATE_SLEEP     22
#define STATE_RUNNING   23
#define STATE_KBD_BLOCKED   -24


/* System call identifiers */
#define SYS_STOP        10
#define SYS_YIELD       11
#define SYS_GETPID      12
#define SYS_PUTS        13
#define SYS_KILL        14
#define SYS_SETPRIO     15
#define SYS_SEND        16
#define SYS_RECV        17
#define SYS_SLEEP       18
#define SYS_SIGHANDLER  19
#define SYS_WAIT        20
#define SYS_RETURN      21
#define SYS_CREATE      22
#define SYS_TIMER       33
#define SYS_DEVICEOPEN  34
#define SYS_DEVICECLOSE 35
#define SYS_DEVICEWRITE 36
#define SYS_DEVICEREAD  37
#define SYS_DEVICEIOCTL 38
#define SYS_CPUTIMES    178

// To return if process is unblocked by a signal during a syscall
#define EINTR           -666

#define TIMER_INT       32
#define KBD_INT         33

#define PREEMPT_INTERVAL  100

#define MILLISECONDS_TICK 10;

/* Structure to track the information associated with a single process */

typedef struct signal_frame{
    funcptr returnAddress;    //return value to set stack too
    funcptr2 handler;
    void* context;
    int signalNumber;
} signalFrame;

typedef struct struct_ps processStatuses;
struct struct_ps {
  int  entries;            // Last entry used in the table
  int  pid[MAX_PROC];      // The process ID
  int  status[MAX_PROC];   // The process status
  long  cpuTime[MAX_PROC]; // CPU time used in milliseconds
};
typedef struct dev_sw devsw;
typedef struct dev_sw {
  int dvnum;
  int (*dvopen)(int , pcb*);
  void (*dvclose)(void);
  int (*dvread)(devsw*, void*, int);
  int (*dvwrite)(devsw* , void* , int );
} devsw;

typedef struct file_descriptor {
    int dvnum;
    devsw* device;
} fd;

struct queue{
    pcb* front;
    pcb* rear;
};

struct struct_pcb {
  unsigned long  *esp;    /* Pointer to top of saved stack           */
  pcb            *next;   /* Next process in the list, if applicable */
  int             state;  /* State the process is in, see above      */
  PID_t           pid;    /* The process's ID                        */
  int             ret;    /* Return value of system call             */
                          /* if process interrupted because of system*/
                          /* call                                    */
  int             sleepTicks; /*The number of clock cycles a PCB has to wait
                                until it is done sleeping */
  long            args;
  int             priority; /* priority of process 0-3; 3 being lowest*/
  unsigned long   startOfMemSpace;  // for kree when stopping a process
  unsigned long   stackSize;    // the size of the proceess' stack - used for checking if args are in bounds of process
  struct queue    senders;    // A queue of senders, i.e. processes waiting to send to this process
  struct queue    receivers;  // A queue of processes waiting to receive from this process
  struct queue    waiters;      // A queue of processes waiting on this process to die`
  long            cpuTime;  /* CPU time consumed                     */
  funcptr2        handlers[31];
  unsigned int    sig_mask;
  int             currentSignalLevel;
  struct queue*   currentQueue;
  fd              fdt[FDT_SIZE];
};

#pragma pack(1)

/* What the set of pushed registers looks like on the stack */
typedef struct context_frame {
  unsigned long        edi;
  unsigned long        esi;
  unsigned long        ebp;
  unsigned long        esp;
  unsigned long        ebx;
  unsigned long        edx;
  unsigned long        ecx;
  unsigned long        eax;
  unsigned long        iret_eip;
  unsigned long        iret_cs;
  unsigned long        eflags;
  unsigned long        stackSlots[];
} context_frame;


/* Memory mangement system functions, it is OK for user level   */
/* processes to call these.                                     */

int      kfree(void *ptr);
void     kmeminit( void );
void     *kmalloc( size_t );
Bool     isValidUserAddress(void* addr);
Bool isValidKernelAddress(void* addr);


/* Internal functions for the kernel, applications must never  */
/* call these.                                                 */
void     dispatch( void );
void     dispatchinit( void );
void     ready( pcb *p );
pcb      *next( void );
void     contextinit( void );
int      contextswitch( pcb *p );
int      create( funcptr fp, size_t stack );
void     set_evec(unsigned int xnum, unsigned long handler);
void     printCF (void * stack);  /* print the call frame */
int      syscall(int call, ...);  /* Used in the system call stub */
int send(pcb* sender, PID_t dest_pid, unsigned long num);
int recv(pcb* receiver, PID_t* src_pid, unsigned int* num);
void tick( struct queue* sleepQueue );
void sleep( pcb* p, unsigned int sleepTimer, struct queue* sleepQueue );
int getCPUtimes(pcb *p, processStatuses *ps);

/* Functions for modifying PCB queues. */
int removeFromQueue(struct queue* q, pcb* p);
void push(struct queue* q, pcb* newElem);
pcb*     pop(struct queue* q);
pcb* getPCBByPID(PID_t pid);
pcb* findInQ(struct queue* q, PID_t pid);


/* Function prototypes for system calls as called by the application */
unsigned int          syscreate( funcptr fp, size_t stack );
void                  sysyield( void );
void                  sysstop( void );
PID_t          sysgetpid( void );
void           sysputs( char *str);
int            syskill( PID_t pid, int signalNumber);
int            syssetprio( int priority );
int            syssend( unsigned int dest_pid, unsigned long num );
int            sysrecv( unsigned int * from_pid, unsigned int * num);
unsigned int   syssleep( unsigned int milliseconds );
int            syswait(int PID);
int            syssighandler(int signal, void (*newhandler)(void *), void (** oldHandler)(void *));
void           sigtramp(void (*handler)(void *), void *cntx);
void           syssigreturn(void *old_sp);
int            sysopen(int device_no);
int            sysclose(int fd);
int            syswrite(int fd, void *buff, int bufflen);
int            sysread(int fd, void *buff, int bufflen);
int            sysioctl(int fd, unsigned long command, ...);
int            getCPUtimes(pcb *p, processStatuses *ps);
int sysgetcputimes(processStatuses *ps) ;


/* Signal handlers. */
void sigreturn(void* context, pcb* p);
int syswait(int PID);
void ignore(void);
void sigsetup(pcb* p);
int signal(pcb* to_kill, int signalNumber);

/* Device Handleres */
void deviceinit( void );
int di_open(int device_no, pcb* p);
int di_close(int fd, pcb* p);
int di_read(int fd, void* buff, int bufflen, pcb* p);
int di_write(int fd, void* buff, int bufflen, pcb* p);
int di_ioctl(int fd, unsigned long command, ...);

int kbdToggleEOF(int fd, unsigned long command, unsigned int new_eof);
int kbdToggleEchoOn(void);
int kbdToggleEchoOff(void);


/* The initial process that the system creates and schedules */
void     root( void );
void     idleproc( void );


/* Functions for testing. */
/*
void test_ToggleEcho(void);
void test_sysioctl(void);
void test_signalPrioritiesHelper(void);
void test_signalPriorities(void);
void test_setsighandler(void);
void test_signalDeadProcess(void);
void test_signalInvalidSignal(void);
void test_signalIdleSignal(void);
void test_setHandler(void);
void test_syswait(void);
void test_sysopen(void);
void test_syswrite(void);
void test_sysread(void);
void test_signalsyscall(void);
void test_interruptRead(void);
void test_signalsyscall(void);
void test_interruptRead(void)
*/

void           set_evec(unsigned int xnum, unsigned long handler);


/* Anything you add must be between the #define and this comment */
#endif

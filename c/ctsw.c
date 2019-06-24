/* ctsw.c : context switcher
 */

#include <xeroskernel.h>
#include <i386.h>


void _syscall_entry_point(void);
void _timer_entry_point(void);
void _kbd_entry_point(void);

static unsigned long      *saveESP;
static int                 interrupt;
static unsigned int        rc;
static long                args;

int contextswitch( pcb *p ) {
/**********************************/

    /* keep every thing on the stack to simplfiy gcc's gesticulations
     */

    saveESP = p->esp;
    rc = p->ret;

    /* In the assembly code, switching to process
     * 1.  Push eflags and general registers on the stack
     * 2.  Load process's return value into eax
     * 3.  load processes ESP into edx, and save kernel's ESP in saveESP
     * 4.  Set up the process stack pointer
     * 5.  store the return value on the stack where the processes general
     *     registers, including eax has been stored.  We place the return
     *     value right in eax so when the stack is popped, eax will contain
     *     the return value
     * 6.  pop general registers from the stack
     * 7.  Do an iret to switch to process
     *
     * Switching to kernel
     * 1.  Push regs on stack, set ecx to 1 if timer interrupt, jump to common
     *     point.
     * 2.  Store request code in ebx
     * 3.  exchange the process esp and kernel esp using saveESP and eax
     *     saveESP will contain the process's esp
     * 4a. Store the request code on stack where kernel's eax is stored
     * 4b. Store the timer interrupt flag on stack where kernel's eax is stored
     * 4c. Store the the arguments on stack where kernel's edx is stored
     * 5.  Pop kernel's general registers and eflags
     * 6.  store the request code, trap flag and args into variables
     * 7.  return to system servicing code
     */
    __asm __volatile( " \
        pushf \n\
        pusha \n\
        movl    rc, %%eax    \n\
        movl    saveESP, %%edx    \n\
        movl    %%esp, saveESP    \n\
        movl    %%edx, %%esp \n\
        movl    %%eax, 28(%%esp) \n\
        popa \n\
        iret \n\
    _timer_entry_point: \n\
        cli \n\
        pusha \n\
        movl %0 , %%ecx \n\
        jmp _common_entry_point \n\
    _kbd_entry_point: \n\
        cli \n\
        pusha \n\
        movl %1 , %%ecx \n\
        jmp _common_entry_point \n\
   _syscall_entry_point: \n\
       cli \n\
       pusha \n\
       movl $0, %%ecx \n\
   _common_entry_point: \n\
        movl    %%eax, %%ebx \n\
        movl    saveESP, %%eax  \n\
        movl    %%esp, saveESP  \n\
        movl    %%eax, %%esp  \n\
        movl    %%ebx, 28(%%esp) \n\
        movl    %%ecx, 24(%%esp) \n\
        movl    %%edx, 20(%%esp) \n\
        popa \n\
        popf \n\
        movl    %%eax, rc \n\
        movl    %%ecx, interrupt \n\
        movl    %%edx, args \n\
        "
        :
        : "i" (TIMER_INT), "i" (KBD_INT)
        : "%eax", "%ebx", "%edx", "%ecx"
    );

    /* save esp and read in the arguments
     */
    p->esp = saveESP;

    if(interrupt){
        p->ret = rc;
        rc = interrupt;
    }else{
        p->args = args;
    }
    return rc;
}

void contextinit( void ) {
/*******************************/

  set_evec( KERNEL_INT, (int) _syscall_entry_point );
  set_evec( TIMER_INT, (int) _timer_entry_point );
  set_evec( KBD_INT, (int) _kbd_entry_point );
  initPIT( PREEMPT_INTERVAL );

}

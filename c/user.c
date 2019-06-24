/* user.c : User processes
 */

#include <xeroskernel.h>
#include <xeroslib.h>

int shellPid;
int alarmMilliseconds;

void alarmProcess(void){
    syssleep(alarmMilliseconds);
    syskill(shellPid, 18);
}

char* stateToString(int state){
  switch(state) {
    case STATE_STOPPED:
      return "STOPPED";
    case STATE_READY:
      return "READY";
    case STATE_BLOCKED:
      return "BLOCKED";
    case STATE_SLEEP:
      return "SLEEPING";
    case STATE_RUNNING:
      return "RUNNING";
    case STATE_KBD_BLOCKED:
      return "BLOCKED";
    default:
      return "STATE UNK";
  }
}

void alarm(void* neverUsed){
    sysputs("\nALARM ALARM ALARM\n");
    funcptr2 notUsed;
    syssighandler(18, NULL, &notUsed);
}

void t(void){
    while(1){
        sysputs("\nT\n");
        syssleep(30000);
        sysyield();
    }
}

char* getAndVerifyString(int fd, char* charBuff, int buffSize, char* targetString){
    sysread(fd, charBuff, buffSize);
    if(strcmp(targetString, charBuff) == 0){
        return charBuff;
    } else {
        return NULL;
    }
}

void handlePSCommand(void){
    processStatuses psTab;
    int procs;
    char buff[100];
    procs = sysgetcputimes(&psTab);
    sysputs("PID     State        Runtime\n");
    for(int j = 0; j <= procs; j++) {
        sprintf(buff, "%4d    %s    %10d\n", psTab.pid[j], stateToString(psTab.status[j]),
        psTab.cpuTime[j]);
        sysputs(buff);
    }
}

int getIntParam(char* buff, int bytes_read){
    char integer[30];
    for(int i = 2; i < bytes_read - 1; i++){
        integer[i - 2] = buff[i];
    }
    int intParam = atoi(integer);
    return intParam;
}

int processCommand(char* command, int bytes_read){
    if(command[0] == 'p' && command[1] == 's' && (command[2] == ' ' || command[2] == '\n')){
        handlePSCommand();
        return 1;
    } else if (command[0] == 'e' && command[1] == 'x' && (command[2] == ' ' || command[2] == '\n')){
        return -1;
    } else if (command[0] == 'k' && command[1] == ' '){
        int pid = getIntParam(command, bytes_read);
        int kill_result = syskill(pid, 31);
        if(kill_result == -514){
            sysputs("No such process\n");
            return -3;
        }
    } else if (command[0] == 'a' && command[1] == ' '){
        alarmMilliseconds = getIntParam(command, bytes_read);
        funcptr2 notUsed;
        syssighandler(18, &alarm, &notUsed);
        PID_t al = syscreate(&alarmProcess, PROC_STACK);
        if(command[bytes_read - 2] != '&'){
            syswait(al);
        }
        return 1;
    } else if (command[0] == 't' && (command[1] == ' ' || command[1] == '\n')){
        int t_pid = syscreate(&t, PROC_STACK);
        if(command[bytes_read - 2] != '&'){
            syswait(t_pid);
        }
        return 1;
    }else{
        // sysputs("\nInvalid Command\n");
        return -2;
    }
    return 1;
}

void shellProgram(void){
    int fd = sysopen(1);
    while(1){
        sysputs("> ");
        char inputBuff[100];
        int bytes_read = sysread(fd, inputBuff, 100);
        if(bytes_read == 0){
            break;
        }
        if(inputBuff[bytes_read-1] != '\n'){
            // if last character isn't \n line was terminated with EOF
            break;
        }
        int result = processCommand(inputBuff, bytes_read);
        if(result == -1){
            //leave the shell program
            break;
        } else if (result == -2){
            //Invalid command;
            sysputs("Command not found\n");
        } else if (result == -10){
            //& case
            continue;
        }
    }
    sysputs("\nGoodbye!\n");
    sysclose(fd);
}

void initProgram(void){
    sysputs("\nWelcome to Xeros - a not so experimental OS\n\n");
    while(1){
        int fd = sysopen(1);
        sysputs("Username: ");
        char userNameBuff[20];
        userNameBuff[0] = 0;
        char* userName = getAndVerifyString(fd, userNameBuff, 20, "cs415\n");
        if(userName == NULL){
            sysputs("\nInvalid Username\n");
            continue;
        } else {
            sysputs("Password: ");
            sysioctl(fd, TOGGLE_DEVICE_ECHO_OFF);
            char passwordBuff[30];
            char* password = getAndVerifyString(fd, passwordBuff, 30, "EveryonegetsanA\n");
            if(password == NULL){
                sysputs("\nInvalid Password\n");
                sysioctl(fd, TOGGLE_DEVICE_ECHO_ON);
                continue;
            } else{
                sysclose(fd);
                sysputs("\nsuccessful login!\n");
                shellPid = syscreate(&shellProgram, PROC_STACK);
                syswait(shellPid);
            }
        }
    }
}

void test_interruptRead(void);
void     root( void ) {
    // test_signalDeadProcess();
    // test_signalInvalidSignal();
    // test_signalIdleSignal();
    // test_setHandler();
    //test_ToggleEcho();
    // test_interruptRead();
    // test_sysread();
    // syscreate(&idleproc, PROC_STACK);
    //test_signalPriorities();
    // test_sysioctl();
    initProgram();

    for(;;);
    sysputs("dont' get here\n");

 }

 void idleproc( void ){
     for(;;){
         __asm __volatile( " \
     		hlt \n\
     			"
     		:
     		:
     		:
     		);
     }
 }

 /* Methods used for testing. */

/*
 void handler1(void* cntx){
     sysputs("In handler 1!\n");
 }

 void handler2(void* cntx){
     sysputs("In handler 2!\n");
 }


void test_ToggleEcho(void){
    int fd = sysopen(1);
    unsigned char buff[20];
    int bytes_read = sysread(fd, buff, 20);
    sysputs("out of sysread\n");
    buff[bytes_read] = '\0';
    sysputs(buff);
    sysputs("toggling echo off");
    sysioctl(fd, TOGGLE_DEVICE_ECHO_OFF);
    sysputs("read2\n");
    bytes_read = sysread(fd, buff, 20);
    sysputs("out of read2\n");
    sysputs(buff);
    sysputs("toggling echo on\n");
    sysioctl(fd, TOGGLE_DEVICE_ECHO_ON);
    sysputs("read3\n");
    bytes_read = sysread(fd, buff, 20);
    sysputs("out of read3\n");
    sysputs(buff);
}



void test_sysioctl(void){
    char message[150];
    int fd = sysopen(1);
    int result = sysioctl(fd, TOGGLE_DEVICE_EOF, 'A');
    sprintf(message, "sysioctl(fd, TOGGLE_DEVICE_EOF, 'A') %d expected 0\n", result);
    sysputs(message);
    result = sysioctl(fd, -1);
    sprintf(message, "sysioctl(fd, -1) %d expected -1\n", result);
    sysputs(message);
    result = sysioctl(fd, TOGGLE_DEVICE_ECHO_ON);
    sprintf(message, "sysioctl(fd, TOGGLE_DEVICE_ECHO_ON) %d expected 0\n", result);
    sysputs(message);
    result = sysioctl(fd, TOGGLE_DEVICE_ECHO_OFF);
    sprintf(message, "sysioctl(fd, TOGGLE_DEVICE_ECHO_OFF) %d expected 0\n", result);
    sysputs(message);
}

void test_signalPrioritiesHelper(void){
    char message[100];
    funcptr2 oldHandler;
    sysputs("in test_signalPrioritiesHelper\n");
    int handler1res = syssighandler(10, &handler1, &oldHandler);
    int handler2res = syssighandler(15, &handler2, &oldHandler);
    syssighandler(3, &handler3, &oldHandler);
    syssighandler(30, &handler30, &oldHandler);
    sprintf(message, "set sig 10 to handler1, sig 15 to handler2, sig 3 to handler3 and sig30 to handler30\n");
    sysputs(message);
    sysputs("waiting on root process now!\n");
    syswait(1);
    sysputs("back in test_signalPrioritiesHelper main, exiting\n");
}

void test_signalPriorities(void){
    sysputs("in test_signalPriorities\n");
    int helperPid = syscreate(&test_signalPrioritiesHelper, PROC_STACK);
    sysyield();
    sysputs("back in test_signalPriorities main\n");
    syskill(helperPid, 10);
    syskill(helperPid, 15);
    syskill(helperPid, 3);
    sysputs("signaeld helper signal 10 then 15 now killing self\n");
    syskill(sysgetpid(), 31);
}


// ------------- Testing signalling. --------------------------


funcptr2 kernelVar;
void test_setsighandler(void){
    syskill(sysgetpid(), 1);    // nothing should happen
    funcptr2 oldHandler;
    int ret = syssighandler(1, oldHandler, &oldHandler);
    char *message;
    sprintf(message, "Result: %d, Expected -2\n", ret);
    sysputs(message);
    ret = syssighandler(32, &handler1, &kernelVar);
    sprintf(message, "Result: %d, Expected -1\n", ret);
    sysputs(message);
    ret = syssighandler(31, &handler1, &kernelVar);
    sprintf(message, "Result: %d, Expected -1\n", ret);
    sysputs(message);
    syskill(sysgetpid(), 1);
}

// Test signalling a process that doesn't exist, with vaild sigNum
void test_signalDeadProcess(void){
    int val = syskill(-1, 7);
    sysputs("test_signalDeadProcess\n");
    char message[100];
    sprintf(message, "Result: %d, Expected -514\n", val);
    sysputs(message);
}


// Test signalling a process that doesn't exist, with vaild sigNum
void test_signalInvalidSignal(void){
    PID_t pid = create(&idleproc, PROC_STACK);
    int val = syskill(pid, -1);
    sysputs("test_signalInvalidSignal\n");
    char message[100];
    sprintf(message, "Result: %d, Expected -583\n", val);
    sysputs(message);
}

void test_signalIdleSignal(void){
    int val = syskill(0,0);
    sysputs("test_signalIdleSignal\n");
    char message[100];
    sprintf(message, "Result: %d, Expected -514\n", val);
    sysputs(message);
}

void test_setHandler(void){
    funcptr2 oldHandler;
    int val = syssighandler(1, &handler1, &oldHandler);
    sysputs("test_setHandler\n");
    char message[100];
    sprintf(message, "syssighandler = %d, Expected: 0\n", val);
    sysputs(message);
    sprintf(message, "handler addr = %d\n", &handler1);
    sysputs(message);
    sprintf(message, "oldHandler = %d, Expected: 0\n", oldHandler);
    sysputs(message);
    syskill(sysgetpid(), 1);
    // for(int i = 0; i<10000000; i++);
    syskill(sysgetpid(), 31);
}


void dummy(void){
    char message[100];
    sprintf(message, "in process %d\n", sysgetpid());
    sysputs(message);
    syssleep(10000);
}

void test_syswait(void){
    PID_t toWait = syscreate(&dummy, PROC_STACK);
    char message[100];
    sprintf(message, "waitng on process %d\n", toWait);
    sysputs(message);
    syswait(toWait);
    sysputs("Done waiting!\n");
}

void test_sysopen(void){
    int ret = sysopen(2);
    char message[100];
    sprintf(message, "Result: %d, Expected -1\n", ret);
    sysputs(message);
}

void test_syswrite(void){
    void* buff = NULL;
    int bufflen = 10;
    // pass invlaid fd
    int ret = syswrite(-1, buff, bufflen);
    char message[100];
    sprintf(message, "Result: %d, Expected -1\n", ret);
    sysputs(message);
}

#include <kbd.h>

void test_sysread(void){
    char buff[2];
    int bufflen = 2;
    int fd = sysopen(0);
    test_sysreadsetup();
    int ret = sysread(fd, buff,bufflen);
    char message[100];
    sprintf(message, "Result: %c, %c\n", buff[0], buff[1]);
    sysputs(message);
    sprintf(message, "Expected: a, b\n");
    sysputs(message);
}

void sigProc(void){
    sysputs("Sleeping in signaller process\n");
    // int ret = syssleep(10000);
    for(volatile int i = 0; i< 10000000;i++);
    char message[100];
    sprintf(message, "Signalling\n");
    sysputs(message);
    syskill(1, 0);
}

void test_signalsyscall(void){
    PID_t proc = syscreate(&sigProc, PROC_STACK);
    funcptr2 oldHandler;
    syssighandler(0, &handler1, &oldHandler);
    int ret = syssend(proc, 1);
    char message[100];
    sprintf(message, "returned: %d, ecxpected: -666\n", ret);
    sysputs(message);
}

void test_interruptRead(void){
    int fd = sysopen(1);
    funcptr2 oldHandler;
    syssighandler(0, &handler1, &oldHandler);
    char buff[100];
    int bufflen = 20;
    syscreate(&sigProc, PROC_STACK);
    int ret = sysread(fd, buff, bufflen);
    char message[100];
    sprintf(message, "returned: %d\n", ret);
    sysputs(message);
    buff[ret] = '\0';
    sysputs(buff);
    sysclose(fd);
}

*/

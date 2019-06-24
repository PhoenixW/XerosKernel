#include <xeroskernel.h>
#include <xeroslib.h>
#include <stdarg.h>
#include "i386.h"

extern devsw devTable[2];

int findOpenFDTEntry(pcb* p){
	for(int i = 0; i < FDT_SIZE; i++){
		if(p->fdt[i].dvnum == -1){
			return i;
		}
	}
	return -1;
}

int di_open(int device_no, pcb* p){
	if(device_no < 0 || device_no >= NUM_DEVICES) return -1;
	int fdt_entry = findOpenFDTEntry(p);
	if(fdt_entry == -1){
		return -1;
	} else {
		p->fdt[fdt_entry].dvnum = devTable[device_no].dvnum;
		p->fdt[fdt_entry].device = &devTable[device_no];
		return p->fdt[fdt_entry].device->dvopen(device_no, p) ? fdt_entry : -1;
	}
}

int di_close(int fd, pcb* p){
	if(fd < 0 || fd > FDT_SIZE - 1){
		return  -1;
	} else if (p->fdt[fd].dvnum == -1) {
		return  -1;
	} else {
		p->fdt[fd].device->dvclose();
		p->fdt[fd].dvnum = -1;
		p->fdt[fd].device = NULL;
		return 0;
	}
}

int di_read(int fd, void* buff, int bufflen, pcb* p){
	if(fd < 0 || fd > FDT_SIZE - 1){
		return  -1;
	} else if (p->fdt[fd].dvnum == -1) {
		return  -1;
	}
	devsw* devptr = p->fdt[fd].device;
	return (devptr->dvread)(devptr, buff, bufflen);
}

int di_write(int fd, void* buff, int bufflen, pcb* p){
	if(fd < 0 || fd > FDT_SIZE - 1){
		return  -1;
	} else if (p->fdt[fd].dvnum == -1) {
		return  -1;
	}
	devsw* devptr = p->fdt[fd].device;
	return (devptr->dvwrite)(devptr, buff, bufflen);
}

int di_ioctl(int fd, unsigned long command, ...){
	if(command == TOGGLE_DEVICE_EOF){
		va_list other_args = (va_list) command;
		unsigned int new_eof = va_arg(other_args, unsigned int);
		return kbdToggleEOF(fd, command, new_eof);
	} else if (command == TOGGLE_DEVICE_ECHO_OFF){
		return kbdToggleEchoOff();
	} else if (command == TOGGLE_DEVICE_ECHO_ON) {
		return kbdToggleEchoOn();
	} else {
		return -1;
	}
}

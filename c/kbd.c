#include <xeroskernel.h>
#include <xeroslib.h>
#include <stdarg.h>
#include <i386.h>
#include "kbd.h"

/*  Normal table to translate scan code  */
unsigned char   kbcode[] = { 0,
          27,  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',
         '0',  '-',  '=', '\b', '\t',  'q',  'w',  'e',  'r',  't',
         'y',  'u',  'i',  'o',  'p',  '[',  ']', '\n',    0,  'a',
         's',  'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';', '\'',
         '`',    0, '\\',  'z',  'x',  'c',  'v',  'b',  'n',  'm',
         ',',  '.',  '/',    0,    0,    0,  ' ' };

/* captialized ascii code table to tranlate scan code */
unsigned char   kbshift[] = { 0,
           0,  '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',  '(',
         ')',  '_',  '+', '\b', '\t',  'Q',  'W',  'E',  'R',  'T',
         'Y',  'U',  'I',  'O',  'P',  '{',  '}', '\n',    0,  'A',
         'S',  'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  '"',
         '~',    0,  '|',  'Z',  'X',  'C',  'V',  'B',  'N',  'M',
         '<',  '>',  '?',    0,    0,    0,  ' ' };
/* extended ascii code table to translate scan code */
unsigned char   kbctl[] = { 0,
           0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
           0,   31,    0, '\b', '\t',   17,   23,    5,   18,   20,
          25,   21,    9,   15,   16,   27,   29, '\n',    0,    1,
          19,    4,    6,    7,    8,   10,   11,   12,    0,    0,
           0,    0,   28,   26,   24,    3,   22,    2,   14,   13 };

static  int     state; /* the state of the keyboard */

// unsigned char   code;
static int extchar(unsigned char code){
   return state &= ~EXTENDED;
}

unsigned char EOF_CHAR;

static Bool in_use;
static int mode;
static int k_buffIndex;
static int k_buffSize;
static char k_buffer[4];
static Bool EOF_seen;
static int active_devicenum;

pcb* p;
unsigned char* p_buff;
int p_bufflen;		// number of bytes to read
int p_bytesRead;		// number of bytes already read


int kbd_open(int device_no, pcb* pcb){
	if(in_use || device_no < 0 || device_no > 1){
		return 0;
	}
  active_devicenum = device_no;
  EOF_CHAR = CTRL_D;
  mode = device_no == MODE_ECHO ? 1 : 0;
	outb(CTRL_PORT, ENABLE_KBD);
	enable_irq(1,0);
	in_use = 1;
	p = pcb;
    EOF_seen = 0;
	return 1;
}

void kbd_close(void){
	outb(CTRL_PORT, DISABLE_KBD);
	enable_irq(1,1);
	in_use = 0;
	p = NULL;
	p_buff = NULL;
    EOF_seen = FALSE;
}

int kbd_write(devsw* dev, void* buff, int bufflen){
	return -1;
}

int kbd_read(devsw* dev, void* buff, int bufflen){
  if(EOF_seen && k_buffSize == 0){
    return 0;
  }
  p_buff = (unsigned char*)buff;
	p_bufflen = bufflen;
	p_bytesRead = 0;

	// Check to see if there is data ready to be read
	// TODO: check if key is EOF
	if(k_buffSize != 0){
		for(; !(k_buffSize == 0 || p_bytesRead == bufflen); k_buffSize--, k_buffIndex = (k_buffIndex+1)%4, p_bytesRead++){
            char temp=  k_buffer[k_buffIndex];
            if(temp != EOF_CHAR){
			       p_buff[p_bytesRead] = k_buffer[k_buffIndex];
            }else{
        	       return p_bytesRead != 0 ? p_bytesRead : 0;
            }
			// p_buff[p_bytesRead] = k_buffer[k_buffIndex];
              if(mode == MODE_ECHO){
                kprintf("%c", k_buffer[k_buffIndex]);
              }
		}
	}

	return p_bytesRead == bufflen ? bufflen : STATE_KBD_BLOCKED;
}

void kbd_isr(void){
	unsigned char data = inb(CTRL_PORT) & 0x1;
	if(data){
        int ascii = kbtoa(inb(DATA_PORT));
        if(!ascii || ascii == NOCHAR){
          return;
        }
    	data = (unsigned char)(ascii);
        //CTRL-D is typed
        if(data == EOF_CHAR){
          outb(CTRL_PORT, DISABLE_KBD);
          enable_irq(1,1);
          EOF_seen = 1;
          if(p_bytesRead != 0){
              p->ret = p_bytesRead;
          }else{
              p->ret = 0;
          }
          removeFromQueue(p->currentQueue, p);
          ready(p);
          p_buff = NULL;
          p_bytesRead = 0;
          return;
        }
    	if(mode == MODE_ECHO){
    		kprintf("%c", data);
    	}
	}else{
		return; // there is no data to read
	}

	if(p_buff == NULL){
        // can only store 4 items in buffer
		if(k_buffSize < 4){
            k_buffIndex = (k_buffIndex+1)%4;
			k_buffer[k_buffIndex] = data;
			k_buffSize++;
		} // else: discard it
	}else{
		p_buff[p_bytesRead++] = data;
        if(p_bytesRead == p_bufflen){
          p_buff = NULL;
          p_bytesRead = 0;
          removeFromQueue(p->currentQueue, p);
          ready(p);
        }
        if(data == '\n'){
          p->ret = p_bytesRead;
          p_buff[p_bytesRead] = '\0';
          removeFromQueue(p->currentQueue, p);
          ready(p);
          p_buff = NULL;
          p_bytesRead = 0;
        }

	}
}

int kbdToggleEOF(int fd, unsigned long command, unsigned int new_eof){
  EOF_CHAR = (unsigned char)new_eof;
  return 0;
}

int kbdToggleEchoOff(void){
  mode = MODE_NORM;
  return 0;
}

int kbdToggleEchoOn(void){
  mode = MODE_ECHO;
  return 0;
}

int kbdSignalReturn(void){
    p_buff = NULL;
    if(p_bytesRead != 0){
        return p_bytesRead;
    }else if(EOF_seen){
        return 0;
    }else{
        return -666;
    }
}

unsigned int kbtoa( unsigned char code )
{
  unsigned int    ch;

  if (state & EXTENDED)
    return extchar(code);
  if (code & KEY_UP) {
    switch (code & 0x7f) {
    case LSHIFT:
    case RSHIFT:
      state &= ~INSHIFT;
      break;
    case CAPSL:
      kprintf("Capslock off detected\n");
      state &= ~CAPSLOCK;
      break;
    case LCTL:
      state &= ~INCTL;
      break;
    case LMETA:
      state &= ~INMETA;
      break;
    }

    return NOCHAR;
  }


  /* check for special keys */
  switch (code) {
  case LSHIFT:
  case RSHIFT:
    state |= INSHIFT;
    return NOCHAR;
  case CAPSL:
    state |= CAPSLOCK;
    kprintf("Capslock ON detected!\n");
    return NOCHAR;
  case LCTL:
    state |= INCTL;
    return NOCHAR;
  case LMETA:
    state |= INMETA;
    return NOCHAR;
  case EXTESC:
    state |= EXTENDED;
    return NOCHAR;
  }

  ch = NOCHAR;

  if (code < sizeof(kbcode)){
    if ( state & CAPSLOCK )
      ch = kbshift[code];
	  else
	    ch = kbcode[code];
  }
  if (state & INSHIFT) {
    if (code >= sizeof(kbshift))
      return NOCHAR;
    if ( state & CAPSLOCK )
      ch = kbcode[code];
    else
      ch = kbshift[code];
  }
  if (state & INCTL) {
    if (code >= sizeof(kbctl))
      return NOCHAR;
    ch = kbctl[code];
  }
  if (state & INMETA)
    ch += 0x80;
  return ch;
}

/*

Testing Methods

void test_sysreadsetup(void){
    k_buffIndex = 0;
    k_buffSize = 4;
    k_buffer[0] = 'a';
    k_buffer[1] = 'b';
    k_buffer[2] = 'c';
    k_buffer[3] = 'd';
}
*/

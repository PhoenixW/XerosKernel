#ifndef KBD_H
#define KBD_H


#define KEY_UP   0x80            /* If this bit is on then it is a key   */
                                 /* up event instead of a key down event */

/* Control code */
#define LSHIFT  0x2a
#define RSHIFT  0x36
#define LMETA   0x38

#define LCTL    0x1d
#define CAPSL   0x3a
/* scan state flags */
#define INCTL           0x01    /* control key is down          */
#define INSHIFT         0x02    /* shift key is down            */
#define CAPSLOCK        0x04    /* caps lock mode               */
#define INMETA          0x08    /* meta (alt) key is down       */
#define EXTENDED        0x10    /* in extended character mode   */
#define ENABLE_KBD      0xae    /* Code to enable the keyboard - to be written to port 0x64. */
#define DISABLE_KBD     0xad    /* Code to disable the keyboard - to be written to port 0x64. */
#define MODE_ECHO       1
#define MODE_NORM       0

#define EXTESC          0xe0    /* extended character escape    */
#define NOCHAR  256

/* Port numbers. */
#define CTRL_PORT       0x64
#define DATA_PORT       0x60

#define CTRL_D			0x04



unsigned int kbtoa( unsigned char code );

int kbd_open(int device_no, pcb* pc);
void kbd_close(void);
int kbd_write(devsw* dev, void* buff, int bufflen);
int kbd_read(devsw* dev, void* buff, int bufflen);
int kbd_read_echo(devsw* dev, void* buff, int bufflen);
void kbd_isr(void);

int kbdSignalReturn(void);


#endif

/* initialize.c - initproc */

#include <i386.h>
#include <xeroskernel.h>
#include <xeroslib.h>

extern	int	entry( void );  /* start of kernel image, use &start    */
extern	int	end( void );    /* end of kernel image, use &end        */
extern  long	freemem; 	/* start of free memory (set in i386.c) */
extern char	*maxaddr;	/* max memory address (set in i386.c)	*/

/************************************************************************/
/***				NOTE:				      ***/
/***								      ***/
/***   This is where the system begins after the C environment has    ***/
/***   been established.  Interrupts are initially DISABLED.  The     ***/
/***   interrupt table has been initialized with a default handler    ***/
/***								      ***/
/***								      ***/
/************************************************************************/

/*------------------------------------------------------------------------
 *  The init process, this is where it all begins...
 *------------------------------------------------------------------------
 */
void initproc( void )				/* The beginning */
{

  kprintf( "\n\nCPSC 415, 2018W2 \n32 Bit Xeros -21.0.0 - even before beta \nLocated at: %x to %x\n",
	   &entry, &end);

  kprintf("Max addr is %d %x\n", maxaddr, maxaddr);

  kmeminit();
  kprintf("memory inited\n");

  dispatchinit();
  kprintf("dispatcher inited\n");

  contextinit();
  kprintf("context inited\n");

  // TODO: INIT device tables
  deviceinit();
  kprintf("device inited\n");

  // The idle process should be created first, so that it has PID 1
  create(idleproc, PROC_STACK);
  // create(test_receiveWhenOnlyProcess, PROC_STACK);
  create( root, PROC_STACK );
  kprintf("create inited\n");

  dispatch();

  kprintf("\n\nWhen your  kernel is working properly ");
  kprintf("this line should never be printed!\n");

  for(;;) ; /* loop forever */

}

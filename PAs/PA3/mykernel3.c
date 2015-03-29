/* mykernel.c: your portion of the kernel
 *
 *	Below are procedures that are called by other parts of the kernel. 
 *	Your ability to modify the kernel is via these procedures.  You may
 *	modify the bodies of these procedures any way you wish (however,
 *	you cannot change the interfaces).  
 */

#include "aux.h"
#include "sys.h"
#include "mykernel3.h"

#define FALSE 0
#define TRUE 1

/*	A sample semaphore table.  You may change this any way you wish.  
 */

static struct 
{
	int valid;	/* Is this a valid entry (was sem allocated)? */
	int value;	/* value of semaphore */

	int head;	/* Initialize variables */
	int tail;
	int semWait[MAXPROCS];	
} semtab[MAXSEMS];

/*	InitSem () is called when kernel starts up. Initialize data
 *	structures (such as the semaphore table) and call any initialization
 *	procedures here. 
 */

void InitSem ()
{
	int s;

	/* modify or add code any way you wish */

	for (s = 0; s < MAXSEMS; s++) 
	{									/* mark all sems free */
		semtab[s].valid = FALSE;		// Initialize variables 
		semtab[s].head = 0;
		semtab[s].tail = 0;
	}
}

/*	MySeminit (p, v) is called by the kernel whenever the system
 *	call Seminit (v) is called.  The kernel passes the initial
 * 	value v, along with the process ID p of the process that called
 *	Seminit.  MySeminit should allocate a semaphore (find a free entry
 *	in semtab and allocate), initialize that semaphore's value to v,
 *	and then return the ID (i.e., index of the allocated entry).  
 */

int MySeminit (p, v)
	int p, v;
{
	int s;

	/* modify or add code any way you wish */

	for (s = 0; s < MAXSEMS; s++) 
	{
		if (semtab[s].valid == FALSE) 
		{
			break;
		}
	}

	if (s == MAXSEMS) 
	{
		Printf ("No free semaphores\n");
		return (-1);
	}

	semtab[s].valid = TRUE;
	semtab[s].value = v;

	return (s);
}

/*	MyWait (p, s) is called by the kernel whenever the system call
 *	Wait (s) is called.  
 */

void MyWait (p, s)
	int p, s;
{
	/* modify or add code any way you wish */
	if (s >= 0 && s < MAXSEMS) 
	{
		semtab[s].value--;
		if (semtab[s].value < 0) 
		{
			// Add to waiting list
			semtab[s].semWait[semtab[s].tail] = p;
			semtab[s].tail = (semtab[s].tail + 1) % MAXPROCS;
			Block(p);
		}
	}
}

/*	MySignal (p, s) is called by the kernel whenever the system call
 *	Signal (s) is called. 
 */

void MySignal (p, s)
	int p, s;
{
	/* modify or add code any way you wish */

	if (s >= 0 && s < MAXSEMS)
	{
		semtab[s].value++;
		if (semtab[s].value <= 0) 
		{
			Unblock(semtab[s].semWait[semtab[s].head]);
			semtab[s].head = (semtab[s].head + 1) % MAXPROCS;
		}
	}
}
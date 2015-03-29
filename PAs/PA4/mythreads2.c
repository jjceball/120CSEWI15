/*	User-level thread system
 *
 */

#include <setjmp.h>

#include "aux.h"
#include "umix.h"
#include "mythreads.h"

static int MyInitThreadsCalled = 0;	/* 1 if MyInitThreads called, else 0 */
static int curr = 0;			/* Position of current thread */
static int next = -1;			/* next thread value */
static int newIndex = -1;		/* New index value */

static struct thread {			/* thread table */
	int valid;			/* 1 if entry is valid, else 0 */
	int id;				/* Keep track of thread ID */
	int pt;
	void (*ft)();
	jmp_buf env;			/* current context */
	jmp_buf newEnv;			/* new context */
} thread[MAXTHREADS];

static int waiting[MAXTHREADS];		/* queue to hold waiting thread ID */

#define STACKSIZE	65536		/* maximum size of thread stack */

//used to shift values in waiting queue
void shiftQueue () {
	for (int i = 0; i < MAXTHREADS; i++) {
		//check for out of bounds 
		waiting[i] = waiting[i+1];
		waiting[i+1] = -1; //make sure the end is not valid
	}
}

/*	MyInitThreads () initializes the thread package. Must be the first
 *	function called by any user program that uses the thread package.  
 */

void MyInitThreads ()
{
	int i;

	if (MyInitThreadsCalled) {                /* run only once */
		Printf ("InitThreads: should be called only once\n");
		Exit ();
	}

	for (i = 0; i < MAXTHREADS; i++) {	/* initialize thread table */
		thread[i].valid = 0;
		thread[i].id = -1;
	}

	//Not valid amount of threads	
	if (MAXTHREADS <= 1) {
		return;
	}

	for (int i = 1; i <= MAXTHREADS; i++) {
			char s[i*STACKSIZE];
		//When setjmp returns the second time and has a new value
			if (setjmp(thread[i].env) != 0) {
				(thread[curr].ft)(thread[curr].pt);
				MyExitThread();	
			}
			if (((int) &s[STACKSIZE-1]) - ((int) &s[0]) + 1 != STACKSIZE) {
				Printf ("Stack space reservation failed\n");
				Exit ();
			}
	}
	
	thread[0].valid = 1;			/* initialize thread 0 */
	
	MyInitThreadsCalled = 1;
}

/*	MySpawnThread (func, param) spawns a new thread to execute
 *	func (param), where func is a function with no return value and
 *	param is an integer parameter.  The new thread does not begin
 *	executing until another thread yields to it.  
 */

int MySpawnThread (func, param)
	void (*func)();		/* function to be executed */
	int param;		/* integer parameter */
{
	if (! MyInitThreadsCalled) {
		Printf ("SpawnThread: Must call InitThreads first\n");
		Exit ();
	}
	
	int validSlot = 0; 
	//check how many empty slots there are
	for (int i = 0; i < MAXTHREADS; i++) {
		validSlot += thread[i].valid;	
	}
	
	//If all slots are full, return -1
	if (validSlot == MAXTHREADS) {
		return -1;
	}

	newIndex = ((newIndex+1)%MAXTHREADS);	
	//Loop through thread table to find last index
	for (int i = 0; i < MAXTHREADS; i++) {
		if (thread[newIndex].valid == 0) { //empty
			thread[newIndex].valid = 1; //reset valid
			thread[newIndex].id = newIndex; //change the id value
			thread[newIndex].ft = func; //saves the function
			thread[newIndex].pt = param; //saves the param
			//put new id in waiting queue
			for (i = 0; i < MAXTHREADS; i++) {
				if (waiting[i] == -1) {
					waiting[newIndex] = newIndex;
					break;
				}
			}
			return (newIndex);
		}
		else { //loop around
			newIndex = ((newIndex+1)%MAXTHREADS);
		}
	}

	//if (setjmp (thread[0].env) == 0) {	/* save context of thread 0 */

		/* The new thread will need stack space.  Here we use the
		 * following trick: the new thread simply uses the current
		 * stack, and so there is no need to allocate space. However,
		 * to ensure that thread 0's stack may grow and (hopefully)
		 * not bump into thread 1's stack, the top of the stack is
		 * effectively extended automatically by declaring a local
		 * variable (a large "dummy" array). This array is never
		 * actually used; to prevent an optimizing compiler from
		 * removing it, it should be referenced.  
		 */

		//char s[STACKSIZE];	/* reserve space for thread 0's stack */
		//void (*ft)() = func;	/* f saves func on top of stack */
		//int pt = param;	/* p saves param on top of stack */
		
		//thread[curr].ft = func;
		//thread[curr].pt = param;

		//if (((int) &s[STACKSIZE-1]) - ((int) &s[0]) + 1 != STACKSIZE) {
			//Printf ("Stack space reservation failed\n");
			//Exit ();
		//}

		//if (setjmp (thread[1].env) == 0) {	/* save context of 1 */
			//longjmp (thread[0].env, 1);	/* back to thread 0 */
		//}
		
		/* here when thread 1 is scheduled for the first time */
		
		//(*ft) (pt);			/* execute func (param) */
		//MyExitThread ();		/* thread 1 is done - exit */
	//}
	
	//Making sure previous environment is overwritten
	memcpy(thread[newIndex].env, thread[newIndex].newEnv, sizeof(jmp_buf));
	
	return (1);		/* done spawning, return new thread ID */
}

/*	MyYieldThread (t) causes the running thread, call it T, to yield to
 *	thread t.  Returns the ID of the thread that yielded to the calling
 *	thread T, or -1 if t is an invalid ID.  Example: given two threads
 *	with IDs 1 and 2, if thread 1 calls MyYieldThread (2), then thread 2
 *	will resume, and if thread 2 then calls MyYieldThread (1), thread 1
 *	will resume by returning from its call to MyYieldThread (2), which
 *	will return the value 2.
 */

int MyYieldThread (t)
	int t;				/* thread being yielded to */
{
	if (! MyInitThreadsCalled) {
		Printf ("YieldThread: Must call InitThreads first\n");
		Exit ();
	}

	if (t < 0 || t >= MAXTHREADS) {
		Printf ("YieldThread: %d is not a valid thread ID\n", t);
		return (-1);
	}
	if (! thread[t].valid) {
		Printf ("YieldThread: Thread %d does not exist\n", t);
		return (-1);
	}

	for (int i = 0; i < MAXTHREADS - 1; i++) {
		//get the thread out of waiting queue
		if (waiting[i] == t) {
			next = waiting[i];
		}

		//put current thread at end of waiting queue
		if (waiting[i] == -1) {
			waiting[i-1] = curr;
		}
	}
	
	shiftQueue();

	if (setjmp (thread[curr].env) == 0) {
		curr = t; //set t to current thread 
                longjmp (thread[t].env, t); //go to next thread
        }
	//else {	
		return next; //return value yielded too
	//}
}

/*	MyGetThread () returns ID of currently running thread.  
 */

int MyGetThread ()
{
	if (! MyInitThreadsCalled) {
		Printf ("GetThread: Must call InitThreads first\n");
		Exit ();
	}
	else {
		return curr;
	}

}

/*	MySchedThread () causes the running thread to simply give up the
 *	CPU and allow another thread to be scheduled. Selecting which
 *	thread to ADrun is determined here. Note that the same thread may
 * 	be chosen (as will be the case if there are no other threads).  
 */

void MySchedThread ()
{
	if (! MyInitThreadsCalled) {
		Printf ("SchedThread: Must call InitThreads first\n");
		Exit ();
	}

	/*if (thread[curr].valid == 0) {
		Exit();
	}

	if (thread[waiting[0]].valid == 0) {
		return;
	}*/

	shiftQueue();

	if (waiting[curr] != -1) {
		MyYieldThread(waiting[curr]);
	}
}

/*	MyExitThread () causes the currently running thread to exit.  
 */

void MyExitThread ()
{
	if (! MyInitThreadsCalled) {
		Printf ("ExitThread: Must call InitThreads first\n");
		Exit ();
	}
	
	shiftQueue();
	//If the whole waiting queue is empty, need to check next one
	if (waiting[1] == -1) {
		Exit();
	}
	
	//update thread table and longjmp to current one
	if (next >= 0 && next != curr) {
		thread[curr].valid = 0; //set to empty
		thread[curr].id = -1;
		curr = next;
		//jump to first in waiting queue
		longjmp(thread[waiting[0]].env, waiting[0]); 
	}
}

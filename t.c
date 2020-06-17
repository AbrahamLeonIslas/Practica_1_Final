/*********** t.c file of A Multitasking System *********/
#include <stdio.h>
#include "type.h"
PROC proc[NPROC]; // NPROC PROCs
PROC *freeList;   // freeList of PROCs
PROC *readyQueue; // priority queue of READY procs
PROC *running;    // current running proc pointer
PROC *sleepList;

#include "queue.c" // include queue.c file
int body();
int kexit();
int kwakeup();
int ksleep();
void des_free_children(PROC *p);
void tswitch();
int event();
void showChild();
void enter_child();
int kwait();


/*******************************************************
kfork() creates a child process; returns child pid.
When scheduled to run, child PROC resumes to body();
********************************************************/
void ksleep(int event)
{
	running->event=event;
	running->status=SLEEP;
	enqueue(&sleepList, running);
	tswitch();
}

int kwait(int *status)
{
	PROC *tmp;
	if(running->child==NULL) 
	{
		return -1;
	}
	else
	{
		for(int i=0;i<NPROC;i++)
		{
			tmp=&proc[i];
			if(tmp->status==ZOMBIE)
			{
				*status=tmp->exitCode;
				tmp->status=FREE;
				tmp->parent=0;
				enqueue(&freeList, tmp);
			}
			return tmp->pid;
		}
		ksleep(running->pid);
	}
}

void kwakeup(int event)
{
	PROC *p, *tmp;
	while(p=dequeue(&sleepList))
	{
		if(p->event == event)
		{
			p=dequeue(&sleepList);
			p->status=READY;
			enqueue(&readyQueue, p);
		}
		else
		{
			enqueue(&sleepList, p);
		}
	}
}

int kfork()
{
  int i;
  PROC *p = dequeue(&freeList);
  if (!p){
    printf("no more proc\n");
    return(-1);
  }

  /* initialize the new proc and its stack */
  p->status = READY;
  p->priority = 1; // ALL PROCs priority=1,except P0
  p->ppid = running->pid;					//ppid= el que esta corriendo

  /************ new task initial stack contents ************
   kstack contains: |retPC|eax|ebx|ecx|edx|ebp|esi|edi|eflag|
                       -1   -2  -3  -4  -5  -6  -7  -8  -9
   **********************************************************/
  for (i=1; i<10; i++)              // zero out kstack cells
    p->kstack[SSIZE - i] = 0;
  p->kstack[SSIZE-1] = (int)body;   // retPC -> body()
  p->ksp = &(p->kstack[SSIZE - 9]); // PROC.ksp -> saved eflag
  enqueue(&readyQueue, p);          // enter p into readyQueue 
  enter_child(&running, p);

  return p->pid;
}/*end kfork()*/

int do_kfork()
{
  int child = kfork();
  if (child < 0)
    printf("kfork failed\n");
  else{
    printf("proc %d kforked a child = %d\n", running->pid, child);
    printList("readyQueue", readyQueue);
  }
  return child;
}/*end do_kfork()*/

int kexit(int exitValue){
	if(exitValue==2){
			running->priority = 0;
			running->exitCode = exitValue;
			running->status = ZOMBIE;
				printf("P%d ", running->pid);
			showChild(running);
			kwakeup(running->ppid);
	tswitch();
	}else{
//TODO
printf("this case is not implemented");
	}

}

/*************** do Ks ***************/

int do_wait()
{
  int child, status;
  child = kwait(&status);
  if (child<0){
    printf("proc %d wait error : no child\n", running->pid);
    return -1;
  }
  printf("proc %d found a ZOMBIE child %d exitValue=%d\n", 
	   running->pid, child, status);
  return child;
}



/*************************************/

int do_switch()
{
  ksleep(1);
 // printList("sleepList", sleepList);
  tswitch();
}

int do_exit()
{
  kexit(2);
}/*end do_exit()*/


int body() // process body function
{
  int c;
  printf("proc %d starts from body()\n", running->pid);
  while(1){
    printf("***************************************\n");
    printf("proc %d running: Parent=%d  ", running->pid,running->ppid);
    showChild(running);
    printf("input a char [f|s|q|w|: ");
    c = getchar(); getchar(); // kill the \r key
    switch(c){
      case 'f': do_kfork(); break;
      case 's': do_switch(); break;
      case 'q': do_exit(); break;
      case 'w': do_wait(); break;
    }
  }
}/*end body()*/

// initialize the MT system; create P0 as initial running process
int init()
{
  int i;
  PROC *p;
  for (i=0; i<NPROC; i++){ // initialize PROCs
    p = &proc[i];
    p->pid = i;            // PID = 0 to NPROC-1
    p->status = FREE;
    p->priority = 0;
    p->next = p+1;
  }
  proc[NPROC-1].next = 0;
  freeList = &proc[0];     // all PROCs in freeList
  readyQueue = 0;          // readyQueue = empty

  // create P0 as the initial running process
  p = running = dequeue(&freeList); // use proc[0]
  p->status = READY;
  p->ppid = 0; // P0 is its own parent
  printList("freeList", freeList);
  printf("init complete: P0 running\n");
}/*end init()*/

/*************** main() function ***************/
int main()
{
  printf("Welcome to the MT Multitasking System\n");
  init(); // initialize system; create and run P0
  kfork(); // kfork P1 into readyQueue
  while(1){
    printf("P0: switch task\n");
    if (readyQueue)
      tswitch();
  }
}/*end main()*/

/*********** scheduler *************/
int scheduler()
{
  printf("proc %d in scheduler()\n", running->pid);
  if (running->status == READY)
    enqueue(&readyQueue, running);
  printList("readyQueue", readyQueue);
  running = dequeue(&readyQueue);
  printf("next running = %d\n", running->pid);
}/*end scheduler()*/

void enter_child(PROC **queue, PROC *p)
{
	PROC *current=*queue;
	if(current==NULL || p==NULL){
	return;
	}else{
		if(current->child==NULL){
			current->child=p;
			return;
			}else{
				current=current->child;
				while(current->sibling){
						current=current->sibling;
					}
					current->sibling=p;
				} 

		}

}

void des_free_children(PROC *p){
	PROC *tmp,*cur;
	if(p->child==NULL){
		return;
		}
	tmp=p->child;
	if(tmp->status==FREE){
	p->child=tmp->sibling;
	return;
	}
	cur=tmp;
	while(tmp=tmp->sibling){
		if(tmp->status==FREE){
			cur->sibling=tmp->sibling;
			return;
			}
		cur=tmp;
		}
}


void showChild(PROC *p){
	PROC *tmp;
	if(p!=NULL){
		printf("Child = ");	
		if(p->child){
			tmp=p->child;
			printf("[%d %s] -> ", tmp->pid, strStatus[tmp->status]);
			if(tmp=tmp->sibling){
			while(tmp!=NULL){
				if(tmp->sibling==NULL){
					printf("[%d %s] -> NULL\n", tmp->pid, strStatus[tmp->status]);
					}else{
						printf("[%d %s] -> ", tmp->pid, strStatus[tmp->status]);
						}
					tmp=tmp->sibling;
					}
				}else{
					printf("NULL\n");
					}
			}else{
				printf("NULL\n");
		}
	}
}

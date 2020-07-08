/**********************************************************************
 * Copyright (c) 2020
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

/* THIS FILE IS ALL YOURS; DO WHATEVER YOU WANT TO DO IN THIS FILE */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "types.h"
#include "list_head.h"

/**
 * The process which is currently running
 */
#include "process.h"
extern struct process *current;

/**
 * List head to hold the processes ready to run
 */
extern struct list_head readyqueue;


/**
 * Resources in the system.
 */
#include "resource.h"
extern struct resource resources[NR_RESOURCES];


/**
 * Monotonically increasing ticks
 */
extern unsigned int ticks;


/**
 * Quiet mode. True if the program was started with -q option
 */
extern bool quiet;


/***********************************************************************
 * Default FCFS resource acquision function
 *
 * DESCRIPTION
 *   This is the default resource acquision function which is called back
 *   whenever the current process is to acquire resource @resource_id.
 *   The current implementation serves the resource in the requesting order
 *   without considering the priority. See the comments in sched.h
 ***********************************************************************/
bool fcfs_acquire(int resource_id)
{
	struct resource *r = resources + resource_id;

	if (!r->owner) {
		/* This resource is not owned by any one. Take it! */
		r->owner = current;
		return true;
	}

	/* OK, this resource is taken by @r->owner. */

	/* Update the current process state */
	current->status = PROCESS_WAIT;

	/* And append current to waitqueue */
	// -> insert current->list after r->waitqueue
	list_add_tail(&current->list, &r->waitqueue);

	/**
	 * And return false to indicate the resource is not available.
	 * The scheduler framework will soon call schedule() function to
	 * schedule out current and to pick the next process to run.
	 */
	return false;
}

/***********************************************************************
 * Default FCFS resource release function
 *
 * DESCRIPTION
 *   This is the default resource release function which is called back
 *   whenever the current process is to release resource @resource_id.
 *   The current implementation serves the resource in the requesting order
 *   without considering the priority. See the comments in sched.h
 ***********************************************************************/
void fcfs_release(int resource_id)
{
	struct resource *r = resources + resource_id;

	/* Ensure that the owner process is releasing the resource */
	assert(r->owner == current);

	/* Un-own this resource */
	r->owner = NULL;

	/* Let's wake up ONE waiter (if exists) that came first */
	if (!list_empty(&r->waitqueue)) {
		struct process *waiter =
				list_first_entry(&r->waitqueue, struct process, list);

		/**
		 * Ensure the waiter  is in the wait status
		 */
		assert(waiter->status == PROCESS_WAIT);

		/**
		 * Take out the waiter from the waiting queue. Note we use
		 * list_del_init() over list_del() to maintain the list head tidy
		 * (otherwise, the framework will complain on the list head
		 * when the process exits).
		 */
		list_del_init(&waiter->list);

		/* Update the process status */
		waiter->status = PROCESS_READY;

		/**
		 * Put the waiter process into ready queue. The framework will
		 * do the rest.
		 */
		list_add_tail(&waiter->list, &readyqueue);
	}
}



#include "sched.h"

/***********************************************************************
 * FIFO scheduler
 ***********************************************************************/
static int fifo_initialize(void)
{
	return 0;
}

static void fifo_finalize(void)
{
}

static struct process *fifo_schedule(void)
{
	struct process *next = NULL; // 다음에 올 프로세스 

	/* You may inspect the situation by calling dump_status() at any time */
	// dump_dump_status();

	/**
	 * When there was no process to run in the previous tick (so does
	 * in the very beginning of the simulation), there will be
	 * no @current process. In this case, pick the next without examining
	 * the current process. Also, when the current process is blocked
	 * while acquiring a resource, @current is (supposed to be) attached
	 * to the waitqueue of the corresponding resource. In this case just
	 * pick the next as well.
	 */
	if (!current || current->status == PROCESS_WAIT) {
		goto pick_next;
	}

	/* The current process has remaining lifetime. Schedule it again */
	if (current->age < current->lifespan) {
		return current;
	}

pick_next:
	/* Let's pick a new process to run next */

	if (!list_empty(&readyqueue)) { 
		/**
		 * If the ready queue is not empty, pick the first process
		 * in the ready queue
		 */
		next = list_first_entry(&readyqueue, struct process, list);

		/**
		 * Detach the process from the ready queue. Note we use list_del_init()
		 * instead of list_del() to maintain the list head tidy. Otherwise,
		 * the framework will complain (assert) on process exit.
		 */
		list_del_init(&next->list);
	}

	/* Return the next process to run */
	return next;
}

struct scheduler fifo_scheduler = {
	.name = "FIFO",
	.acquire = fcfs_acquire,
	.release = fcfs_release,
	.initialize = fifo_initialize,
	.finalize = fifo_finalize,
	.schedule = fifo_schedule,
};


/***********************************************************************
 * SJF scheduler
 ***********************************************************************/
static struct process *sjf_schedule(void)
{
	/**
	 * Implement your own SJF scheduler here.
	 */

	struct process *next = NULL; // 다음에 올 프로세스 
	struct process *minProcess = NULL; // life span이 제일 짧은 process

	//dump_status();
	if (!current || current->status == PROCESS_WAIT) {
		goto pick_next;
	}

	/* The current process has remaining lifetime. Schedule it again */
	if (current->age < current->lifespan) {
		return current;
	}

pick_next:
	/* Let's pick a new process to run next */

	if (!list_empty(&readyqueue)) {
		
		
		minProcess = list_first_entry(&readyqueue, struct process, list);

		list_for_each_entry(next, &readyqueue, list) { // iterate over list of given type
			if(minProcess->lifespan > next->lifespan) {
				//temp -> lifespan = next->lifespan;
				//minProcess->lifespan = temp->lifespan;
				minProcess = next; // minprocess, next는 process임을 인지할 것.
			}
		}
		
		list_del_init(&minProcess->list);
	}

	/* Return the min process to run */
	return minProcess;
}

struct scheduler sjf_scheduler = {
	.name = "Shortest-Job First",
	.initialize = fifo_initialize,
	.finalize = fifo_finalize,
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	.schedule = sjf_schedule,		 /* TODO: Assign sjf_schedule()
								to this function pointer to activate
								SJF in the system */
	
	// sjf_schedule이 아닌 NULL -> core dumped 
};


/***********************************************************************
 * SRTF scheduler
 ***********************************************************************/
static struct process *srtf_schedule(void) {
	/**
	 * Implement your own SRTF scheduler here.
	 */

	struct process *next = NULL; 
	struct process *minProcess = NULL; 

	//dump_status();
	if (!current || current->status == PROCESS_WAIT) {
		goto pick_next;
	}

	if (current->age < current->lifespan) { // curent process 1(1/3), process 5 (0/1) 인 경우 대비

		list_add_tail(&current->list, &readyqueue); // Insert a new entry after the specified head. 
	}

pick_next:

	if (!list_empty(&readyqueue)) {
		
		
		minProcess = list_first_entry(&readyqueue, struct process, list);

		list_for_each_entry(next, &readyqueue, list) { // iterate over list of given type
			// remaining time이 더 짧다면
			if(minProcess->lifespan - minProcess->age > next->lifespan - next->age) {
				
				minProcess = next; // minprocess, next는 process임을 인지할 것.
			}
		}
		
		list_del_init(&minProcess->list);
	}

	/* Return the min process to run */
	return minProcess;
}

struct scheduler srtf_scheduler = {
	.name = "Shortest Remaining Time First",
	.initialize = fifo_initialize,
	.finalize = fifo_finalize,
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	.schedule = srtf_schedule,
	/* You need to check the newly created processes to implement SRTF.
	 * Use @forked() callback to mark newly created processes */
	/* Obviously, you should implement srtf_schedule() and attach it here */


};


/***********************************************************************
 * Round-robin scheduler
 ***********************************************************************/
static struct process *rr_schedule(void) {
	/**
	 * Implement your own rr scheduler here.
	 */

	struct process *next = NULL; 
	struct process *temp = NULL;

	if (!current || current->status == PROCESS_WAIT) {
		goto pick_next;
	}

	
	if (current->age < current->lifespan) { // SRTF와 동일. 
		
		list_add_tail(&current->list, &readyqueue); 
	}

pick_next:
	if (!list_empty(&readyqueue)) { // time quantum : 1 / __do_simulation을 바탕으로 돌리기만 하면 됨.
		next = list_first_entry(&readyqueue, struct process, list); 
		list_del_init(&next->list); 
	}
	return next;
}

struct scheduler rr_scheduler = {
	.name = "Round-Robin",
	.initialize = fifo_initialize,
	.finalize = fifo_finalize,
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	.schedule = rr_schedule,
	/* Obviously, you should implement rr_schedule() and attach it here */
};


/***********************************************************************
 * Priority scheduler
 ***********************************************************************/
static struct process *prio_schedule(void) {
	/**
	 * Implement your own prio scheduler here.
	 */
	struct process *next = NULL; //  
	struct process *prioHighestProcess = NULL; 

	if (!current || current->status == PROCESS_WAIT) {
		goto pick_next;
	}

	if (current->age < current->lifespan) {
		list_add_tail(&current->list, &readyqueue); 
	}

pick_next:
	/* Let's pick a new process to run next */

	if (!list_empty(&readyqueue)) { 
		
		prioHighestProcess = list_first_entry(&readyqueue, struct process, list);

		list_for_each_entry(next, &readyqueue, list) {
			if(prioHighestProcess->prio < next->prio) { // 우선순위가 제일 높은 process 찾기
				
				prioHighestProcess = next; 
			}
		}
		
		list_del_init(&prioHighestProcess->list);
	}

	/* Return the next process to run */
	return prioHighestProcess;
}

struct scheduler prio_scheduler = {
	.name = "Priority",
	.acquire = fcfs_acquire, 
	.release = fcfs_release, 
	.schedule = prio_schedule,
	/**
	 * Implement your own acqure/release function to make priority
	 * scheduler correct.
	 */
	/* Implement your own prio_schedule() and attach it here */
};


/***********************************************************************
 * Priority scheduler with priority ceiling protocol
 ***********************************************************************/
bool pcp_acquire(int resource_id)
{
	struct resource *r = resources + resource_id;

	if (!r->owner) {
		/* This resource is not owned by any one. Take it! */
		r->owner = current;
		current->prio  = MAX_PRIO;
		return true;
	}

	/* OK, this resource is taken by @r->owner. */

	/* Update the current process state */
	current->status = PROCESS_WAIT;

	/* And append current to waitqueue */
	list_add_tail(&current->list, &r->waitqueue);

	/**
	 * And return false to indicate the resource is not available.
	 * The scheduler framework will soon call schedule() function to
	 * schedule out current and to pick the next process to run.
	 */
	return false;
}

void pcp_release(int resource_id)
{
	struct resource *r = resources + resource_id;
	struct process *next = NULL;
	struct process *prioHighestProcess = NULL;

	
	/* Ensure that the owner process is releasing the resource */
	assert(r->owner == current);

	r->owner->prio = r->owner->prio_orig;
	
	/* Un-own this resource */
	r->owner = NULL;

	/* Let's wake up ONE waiter (if exists) that came first */
	if (!list_empty(&r->waitqueue)) {
		struct process *waiter =
				list_first_entry(&r->waitqueue, struct process, list);

		/**
		 * Ensure waiter status is in the wait status
		 */
		assert(waiter->status == PROCESS_WAIT);

		/**
		 * Take out the waiter from the waiting queue. Note we use
		 * list_del_init() over list_del() to maintain the list head tidy
		 * (otherwise, the framework will complain on the list head
		 * when the process exits).
		 */
		list_del_init(&waiter->list);

		/* Update the process status */
		waiter->status = PROCESS_READY;

		/**
		 * Put the waiter process into ready queue. The framework will
		 * do the rest.
		 */
		list_add_tail(&waiter->list, &readyqueue);
	}
}

static struct process *pcp_schedule(void) {
	/**
	 * Implement your own pcp scheduler here.
	 */
	struct process *next = NULL; //  
	struct process *prioHighestProcess = NULL; 

	if (!current || current->status == PROCESS_WAIT) {
		goto pick_next;
	}

	if (current->age < current->lifespan) {

		list_add_tail(&current->list, &readyqueue); // 14 ~ segmentation  fault (core dumped) ////
		
	}

pick_next:
	/* Let's pick a new process to run next */

	if (!list_empty(&readyqueue)) { 
		
		prioHighestProcess = list_first_entry(&readyqueue, struct process, list);

		list_for_each_entry(next, &readyqueue, list) {
			if(prioHighestProcess->prio < next->prio) { // 우선순위가 제일 높은 process 찾기
				
				prioHighestProcess = next; 
			}
		}
		
		list_del_init(&prioHighestProcess->list);
	}

	/* Return the  prioHighestProcess to run */
	return prioHighestProcess;
}

struct scheduler pcp_scheduler = {
	.name = "Priority + Priority Ceiling Protocol",
	.acquire = pcp_acquire, 
	.release = pcp_release, 
	.schedule = pcp_schedule,
	/**
	 * Implement your own acqure/release function too to make priority
	 * scheduler correct.
	 */
};


/***********************************************************************
 * Priority scheduler with priority inheritance protocol
 ***********************************************************************/
bool pip_acquire(int resource_id)
{
	struct resource *r = resources + resource_id;

	if (!r->owner) {
		/* This resource is not owned by any one. Take it! */
		r->owner = current;
		return true;
	}

	if(r->owner->prio < current->prio) {
		r->owner->prio = current->prio;
		current->status = PROCESS_WAIT;
		list_add_tail(&current->list, &r->waitqueue);

		return false;
	}
	/* OK, this resource is taken by @r->owner. */

	/* Update the current process state */
	current->status = PROCESS_WAIT;

	/* And append current to waitqueue */
	list_add_tail(&current->list, &r->waitqueue);

	/**
	 * And return false to indicate the resource is not available.
	 * The scheduler framework will soon call schedule() function to
	 * schedule out current and to pick the next process to run.
	 */
	return false;
}

void pip_release(int resource_id)
{
	struct resource *r = resources + resource_id;
	struct process *next = NULL;
	struct process *prioHighestProcess = NULL;

	/* Ensure that the owner process is releasing the resource */
	assert(r->owner == current);
	r->owner->prio = current->prio_orig;

	/* Un-own this resource */
	r->owner = NULL;

	/* Let's wake up ONE waiter (if exists) that came first */
	if (!list_empty(&r->waitqueue)) {
		
		prioHighestProcess = list_first_entry(&r->waitqueue, struct process, list);

		list_for_each_entry(next, &r->waitqueue, list) {
			if(prioHighestProcess->prio < next->prio) { // 우선순위가 제일 높은 process 찾기
				
				prioHighestProcess = next; 
			}
		}
		/**
		 * Ensure max_process status is in the wait status
		 */
		assert(prioHighestProcess->status == PROCESS_WAIT);

		/**
		 * Take out the waiter from the waiting queue. Note we use
		 * list_del_init() over list_del() to maintain the list head tidy
		 * (otherwise, the framework will complain on the list head
		 * when the process exits).
		 */
		list_del_init(&prioHighestProcess->list);

		/* Update the process status */
		prioHighestProcess->status = PROCESS_READY;

		/**
		 * Put the waiter process into ready queue. The framework will
		 * do the rest.
		 */
		list_add_tail(&prioHighestProcess->list, &readyqueue);
	}
}

static struct process *pip_schedule(void) {
	/**
	 * Implement your own pcp scheduler here.
	 */
	struct process *next = NULL; //  
	struct process *prioHighestProcess = NULL; 

	//dump_status();
	if (!current || current->status == PROCESS_WAIT) {
		goto pick_next;
	}

	if (current->age < current->lifespan) {
		
		list_add_tail(&current->list, &readyqueue);
	}

pick_next:
	/* Let's pick a new process to run next */

	if (!list_empty(&readyqueue)) { 
		
		prioHighestProcess = list_first_entry(&readyqueue, struct process, list);

		list_for_each_entry(next, &readyqueue, list) {
			if(prioHighestProcess->prio < next->prio) { // 우선순위가 제일 높은 process 찾기
				
				prioHighestProcess = next; 
			}
		}
		
		list_del_init(&prioHighestProcess->list);
	}

	/* Return the next process to run */
	return prioHighestProcess;
}

struct scheduler pip_scheduler = {
	.name = "Priority + Priority Inheritance Protocol",
	.acquire = pip_acquire, 
	.release = pip_release, 
	.schedule = pip_schedule,
	/**
	 * Ditto
	 */
};

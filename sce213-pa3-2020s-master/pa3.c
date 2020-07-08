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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include "types.h"
#include "locks.h"
#include "atomic.h"
#include "list_head.h"


/*********************************************************************
 * Spinlock implementation
 *********************************************************************/
struct spinlock {
	int held; // int held = 0; -> X
};

/*********************************************************************
 * init_spinlock(@lock)
 *
 * DESCRIPTION
 *   Initialize your spinlock instance @lock
 */
void init_spinlock(struct spinlock *lock)
{
	lock->held = 0;
	return;
}

/*********************************************************************
 * acqure_spinlock(@lock)
 *
 * DESCRIPTION
 *   Acquire the spinlock instance @lock. The returning from this
 *   function implies that the calling thread grapped the lock.
 *   In other words, you should not return from this function until
 *   the calling thread gets the lock.
 */
void acquire_spinlock(struct spinlock *lock)
{
	// atomic instruction. Test-and-set에 old==expected 추가.
	// update the memory location with the new value only when its old value equals to the
	// "expected" value
	while(compare_and_swap(&lock->held, 0, 1)); 
	return;
}

/*********************************************************************
 * release_spinlock(@lock)
 *
 * DESCRIPTION
 *   Release the spinlock instance @lock. Keep in mind that the next thread
 *   can grap the @lock instance right away when you mark @lock available;
 *   any pending thread may grap @lock right after marking @lock as free
 *   but before returning from this function.
 */
void release_spinlock(struct spinlock *lock)
{
	lock->held = 0;
	return;
}


/********************************************************************

 * Blocking mutex implementation

 ********************************************************************/

struct thread
{
	pthread_t pthread;
	struct list_head list;
};

struct mutex
{
	int S;
	struct list_head Q;
	struct spinlock wait_lock; //
};
/********************************************************************

 * Blocking semaphore implementation

 ********************************************************************/

struct semaphore
{
	int S;
	struct list_head Q;
	struct spinlock wait_lock; //
	int held;
};

struct semaphore MUTEX;
struct semaphore full;
struct semaphore empty;

/*********************************************************************
 * init_semaphore(@semaphore)
 *
 * DESCRIPTION
 *   Initialize the semaphore instance pointed by @semaphore.
 */

void init_semaphore(struct semaphore *semaphore, int S)
{	
	INIT_LIST_HEAD(&semaphore->Q); 
	semaphore->S = 1; // 1로 초기화
	semaphore->wait_lock; // semaphore의 spinlock
	return;
}

/*********************************************************************
 * wait_semaphore(@semaphore)
  * DESCRIPTION
 *   Acquire the mutex instance @mutex. Likewise acquire_spinlock(), you
 *   should not return from this function until the calling thread gets the
 *   mutex instance. But the calling thread should be put into sleep when
 *   the mutex is acquired by other threads.
 *
 * HINT
 *   1. Use sigwaitinfo(), sigemptyset(), sigaddset(), sigprocmask() to
 *      put threads into sleep until the mutex holder wakes up
 *   2. Use pthread_self() to get the pthread_t instance of the calling thread.
 *   3. Manage the threads that are waiting for the mutex instance using
 *      a custom data structure containing the pthread_t and list_head.
 *      However, you may need to use a spinlock to prevent the race condition
 *      on the waiter list (i.e., multiple waiters are being inserted into the 
 *      waiting list simultaneously, one waiters are going into the waiter list
 *      and the mutex holder tries to remove a waiter from the list, etc..)
 */
void wait_semaphore(struct semaphore *semaphore)
{
	int signum;
	siginfo_t info;
	sigset_t set; // 시그널
	struct thread *thr;
	
	while(compare_and_swap(&semaphore->wait_lock.held, 0, 1));
	//acquire_spinlock(&(semaphore->wait_lock));
	semaphore->S--;
	if (semaphore->S < 0) {
		
		sigemptyset(&set);
		sigaddset(&set, SIGINT); // SIGINT 시그널 번호를 set에 추가
		// 기존에 블록된 시그널이 있다면 set signal을 추가 / 초기화라고 생각하기
		sigprocmask(SIG_BLOCK, &set, NULL); 
		
		thr = malloc(sizeof(struct thread));
		thr->pthread = pthread_self();


		list_add_tail(&thr->list, &semaphore->Q); // thread를 semaphore->Q에 넣기
		
		//release_spinlock(&semaphore->wait_lock);
		semaphore->wait_lock.held=0;

		signum = sigwaitinfo(&set, &info);
		// 시그널 번호 SIGINT를 받아야함. 그러지 않으면 race condition 발생 
		while(signum != SIGINT);
			//if (signum == SIGINT) {
				// 2. Verify the mutual exclusiveness further..세그멘테이션 오류 (core dumped)
				// 여러 thread들이 동시에 진행되지 못하도록 
		while(compare_and_swap(&semaphore->wait_lock.held, 0, 1));
		free(thr);
		
	}
	//release_spinlock(&semaphore->wait_lock);
	semaphore->wait_lock.held=0;
	return;
}

/*********************************************************************
 * signal_semaphore(@semaphore)
 *
 * DESCRIPTION
 *   Release the semaphore held by the calling thread.
 *
 * 
 */
void signal_semaphore(struct semaphore *semaphore)
{
	struct thread *thr;
	//acquire_spinlock(&(semaphore->wait_lock));
	while(compare_and_swap(&semaphore->wait_lock.held, 0, 1));
	semaphore->S++;
	
	if (semaphore->S <= 0)
	{
		thr = list_first_entry(&semaphore->Q, struct thread, list);
		list_del_init(&thr->list);
		pthread_kill(thr->pthread, SIGINT);
		
		//release_spinlock(&mutex->wait_lock); // 여기에 할 시 race condition
	}
	//release_spinlock(&semaphore->wait_lock);
	semaphore->wait_lock.held = 0;
	return;
}

/*********************************************************************
 * init_mutex(@semaphore)
 *
 * DESCRIPTION
 *   Initialize the semaphore instance pointed by @mutex.
 */

void init_mutex(struct mutex *mutex)
{
	//printf("error??");
	//INIT_LIST_HEAD(&mutex->Q);
	
	init_semaphore(&MUTEX, 1);
	init_semaphore(&empty, 0);
	//mutex->S = 1;
	mutex->S = 0;
	//printf("%d\n", mutex->S);
	return;
}

/*********************************************************************
 * acquire_mutex(@mutex)
 *
 * DESCRIPTION
 *   Acquire the mutex instance @mutex. Likewise acquire_spinlock(), you
 *   should not return from this function until the calling thread gets the
 *   mutex instance. But the calling thread should be put into sleep when
 *   the mutex is acquired by other threads.
 *
 * HINT
 *   1. Use sigwaitinfo(), sigemptyset(), sigaddset(), sigprocmask() to
 *      put threads into sleep until the mutex holder wakes up
 *   2. Use pthread_self() to get the pthread_t instance of the calling thread.
 *   3. Manage the threads that are waiting for the mutex instance using
 *      a custom data structure containing the pthread_t and list_head.
 *      However, you may need to use a spinlock to prevent the race condition
 *      on the waiter list (i.e., multiple waiters are being inserted into the 
 *      waiting list simultaneously, one waiters are going into the waiter list
 *      and the mutex holder tries to remove a waiter from the list, etc..)
 */

void acquire_mutex(struct mutex *mutex)
{
	//printf("%d ", mutex->S);
	//again :
	wait_semaphore(&MUTEX);
	mutex->S--;
	printf("%d ", mutex->S);
	if(mutex->S < 0){
		signal_semaphore(&MUTEX);
		wait_semaphore(&empty);
		//goto again;
	}
	else
		signal_semaphore(&MUTEX);
	return;
}



/*********************************************************************
 * release_mutex(@mutex) using semaphore
 *
 * DESCRIPTION
 *   Release the semaphore held by the calling thread.
 *
 * HINT
 *   1. Use pthread_kill() to wake up a waiter thread
 *   2. Be careful to prevent race conditions while accessing the waiter list
 */

void release_mutex(struct mutex *mutex)
{
	wait_semaphore(&MUTEX);
	mutex->S++;

	if(mutex->S <= 0)
		signal_semaphore(&empty);
	//else -> race condition 

	signal_semaphore(&MUTEX);	
	return;
}

/*********************************************************************
 * Ring buffer
 *********************************************************************/
struct ringbuffer {
	/** NEVER CHANGE @nr_slots AND @slots ****/
	/**/ int nr_slots;                     /**/
	/**/ int *slots;                       /**/
	/*****************************************/
	int held;
	int count;
	int in; 
	int out;
	//struct spinlock spin;
};

struct ringbuffer ringbuffer = {
	
};


/*********************************************************************
 * enqueue_into_ringbuffer(@value) using semaphore
 *
 * DESCRIPTION
 *   Generator in the framework tries to put @value into the buffer.
 */
void enqueue_into_ringbuffer(int value)
{
	again:
	wait_semaphore(&full); 
	wait_semaphore(&MUTEX);

	if (ringbuffer.count == ringbuffer.nr_slots) {
		signal_semaphore(&MUTEX);
		signal_semaphore(&empty);
		goto again;
	}
	
	*(ringbuffer.slots +ringbuffer.in) = value;
	ringbuffer.in = (ringbuffer.in + 1) % ringbuffer.nr_slots;
	ringbuffer.count++; 

	signal_semaphore(&MUTEX);
	signal_semaphore(&empty); 

	return;
}


/*********************************************************************
 * dequeue_from_ringbuffer(@value) using semaphore
 *
 * DESCRIPTION
 *   Counter in the framework wants to get a value from the buffer.
 *
 * RETURN
 *   Return one value from the buffer.
 */
int dequeue_from_ringbuffer(void)
{
	int data;
	again:
	wait_semaphore(&empty);
	wait_semaphore(&MUTEX);

	if (ringbuffer.count == 0) {
		signal_semaphore(&MUTEX);
		signal_semaphore(&full);
		goto again;
	}

	data = *(ringbuffer.slots + ringbuffer.out);
	ringbuffer.out = (ringbuffer.out + 1) % ringbuffer.nr_slots;
	ringbuffer.count--;

	signal_semaphore(&MUTEX);
	signal_semaphore(&full);

	return data;
}

/*********************************************************************
 * fini_ringbuffer
 *
 * DESCRIPTION
 *   Clean up your ring buffer.
 */
void fini_ringbuffer(void)
{
	free(ringbuffer.slots);
	
}

/*********************************************************************
 * init_ringbuffer(@nr_slots)
 *
 * DESCRIPTION
 *   Initialize the ring buffer which has @nr_slots slots.
 *
 * RETURN
 *   0 on success.
 *   Other values otherwise.
 */
int init_ringbuffer(const int nr_slots)
{
	/** DO NOT MODIFY THOSE TWO LINES **************************/
	/**/ ringbuffer.nr_slots = nr_slots;                     /**/
	/**/ ringbuffer.slots = malloc(sizeof(int) * nr_slots);  /**/
	/***********************************************************/

	ringbuffer.count = 0;
	ringbuffer.held = 0;
	ringbuffer.in = 0;
	ringbuffer.out = 0;

	init_semaphore(&MUTEX, 1);
	init_semaphore(&empty, 0);
	init_semaphore(&full, nr_slots);
	return 0;
}

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
#include <strings.h>

#include "types.h"
#include "list_head.h"
#include "vm.h"

/**
 * Ready queue of the system
 */


/**
 * Simplified PCB
 */

/*
struct process {
	unsigned int pid;

	struct pagetable pagetable;

	struct list_head list;  /* List head to chain processes on the system */
//}; 


extern struct list_head processes;

/**
 * Initial process
 */

/*static struct process init = {
	.pid = 0,
	.list = LIST_HEAD_INIT(init.list),
	.pagetable = {
		.outer_ptes = { NULL },
	},
};*/


/**
 * Currently running process
 * 
 */
extern struct process *current;

/**
 * Page Table Base Register that MMU will walk through for address translation
 */
extern struct pagetable *ptbr;


/**
 * The number of mappings for each page frame. Can be used to determine how
 * many processes are using the page frames.
 */
extern unsigned int mapcounts[];


/**
 * alloc_page(@vpn, @rw)
 *
 * DESCRIPTION
 *   Allocate a page frame that is not allocated to any process, and map it
 *   to @vpn. When the system has multiple free pages, this function should
 *   allocate the page frame with the **smallest pfn**.
 *   You may construct the page table of the @current process. When the page
 *   is allocated with RW_WRITE flag, the page may be later accessed for writes.
 *   However, the pages populated with RW_READ only should not be accessed with
 *   RW_WRITE accesses.
 *
 * RETURN
 *   Return allocated page frame number.
 *   Return -1 if all page frames are allocated.
 */
unsigned int alloc_page(unsigned int vpn, unsigned int rw)
{
	// this function should allocate the page frame with the **smallest pfn**.
	int num = 0;
	int i = 0;
	int out = vpn / 16; // pd_index
	int in = vpn % 16; // pte_index

	//int in = vpn;
	//printf("%d %d\n", in, out);

	if(current->pagetable.outer_ptes[out] == NULL){
			struct pte_directory *pd;
			pd = (struct pte_directory *)malloc(sizeof(struct pte_directory));
			current->pagetable.outer_ptes[out] = pd;
	}
	//printf("%lu", sizeof(struct pte_directory));
	//printf("1\n");
	while(true) {
		if(mapcounts[i] == 16)
			return -1;
		if(mapcounts[i] == 0) { // page frame에 매핑하는 카운트가 0일 때 
			
			current->pagetable.outer_ptes[out]->ptes[in].pfn=i;
			current->pagetable.outer_ptes[out]->ptes[in].valid=true;
			
		
			if(rw == RW_READ) // RW_READ일 때는 write permission 꺼둬야함
				current->pagetable.outer_ptes[out]->ptes[in].writable = false;
			else
				current->pagetable.outer_ptes[out]->ptes[in].writable=true;

			// writable에대한 정보를 private에 담아둔다.
			current->pagetable.outer_ptes[out]->ptes[in].private = current->pagetable.outer_ptes[out]->ptes[in].writable;
			mapcounts[i]++;
			num=i;
			//printf("%d\n", mapcounts[i]);
			
			break;
		}
		i++;
	}
	//printf("2\n");
	return num;
}


/**
 * free_page(@vpn)
 *
 * DESCRIPTION
 *   Deallocate the page from the current processor. Make sure that the fields
 *   for the corresponding PTE (valid, writable, pfn) is set @false or 0.
 *   Also, consider carefully for the case when a page is shared by two processes,
 *   and one process is to free the page.
 */
void free_page(unsigned int vpn)
{
	int out = vpn / 16;
	int in = vpn % 16;

	int n = current->pagetable.outer_ptes[out]->ptes[in].pfn;
	//printf("%d %d\n", out, n);
	current->pagetable.outer_ptes[out]->ptes[in].valid=false;
	current->pagetable.outer_ptes[out]->ptes[in].writable=false;			
	current->pagetable.outer_ptes[out]->ptes[in].pfn=0;
	mapcounts[n]--;			
	
	return;
}


/**
 * handle_page_fault()
 *
 * DESCRIPTION
 *   Handle the page fault for accessing @vpn for @rw. This function is called
 *   by the framework when the __translate() for @vpn fails. This implies;
 *   0. page directory is invalid
 *   1. pte is invalid
 *   2. pte is not writable but @rw is for write
 *   This function should identify the situation, and do the copy-on-write if
 *   necessary.
 *
 * RETURN
 *   @true on successful fault handling
 *   @false otherwise
 */
bool handle_page_fault(unsigned int vpn, unsigned int rw)
{
	int out = vpn / 16;
	int in = vpn % 16;

	if((current->pagetable.outer_ptes[out] != NULL) && (current->pagetable.outer_ptes[out]->ptes[in].private==true) &&
		 (current->pagetable.outer_ptes[out]->ptes[in].valid == true) && rw!=RW_READ) {

		// alloc_page에서 rw != RW_READ일 때 writable true
		current->pagetable.outer_ptes[out]->ptes[in].writable=true; 
		// pfn이 2 이상이면 시작
		if(mapcounts[current->pagetable.outer_ptes[out]->ptes[in].pfn]>=2){

			for(int i=0;i<16;i++){
				if(mapcounts[i]==16){
					return false;
				}

				if(mapcounts[i]==0){
					mapcounts[current->pagetable.outer_ptes[out]->ptes[in].pfn]--;
					current->pagetable.outer_ptes[out]->ptes[in].pfn=i;
					
					mapcounts[i]++;
					return true;
				}	
			}
		}
		
		return true;
	}
	
	else
		return false;
	
	
}


/**
 * switch_process()
 *
 * DESCRIPTION
 *   If there is a process with @pid in @processes, switch to the process.
 *   The @current process at the moment should be put into the @processes
 *   list, and @current should be replaced to the requested process.
 *   Make sure that the next process is unlinked from the @processes, and
 *   @ptbr is set properly.
 *
 *   If there is no process with @pid in the @processes list, fork a process
 *   from the @current. This implies the forked child process should have
 *   the identical page table entry 'values' to its parent's (i.e., @current)
 *   page table. 
 *   To implement the copy-on-write feature, you should manipulate the writable
 *   bit in PTE and mapcounts for shared pages. You may use pte->private for 
 *   storing some useful information :-)
 */
void switch_process(unsigned int pid)
{
	
	struct process *cur;
	list_for_each_entry(cur,&processes,list){	
		if(cur->pid == pid){ // 해당하는 pid가 맞다면 
			list_add_tail(&current->list,&processes);
			list_del_init(&cur->list);

			ptbr = &cur->pagetable; 
			current = cur; // current fork
			return;
		}
	}
	
	
	struct process *np = (struct process *)malloc(sizeof(struct process));
	np->pid = pid;

	for(int i=0;i<16;i++){
		if(current->pagetable.outer_ptes[i]==NULL) 
			continue;

		else { // outer_ptes[i]가 NULL이 아닐 때
			np->pagetable.outer_ptes[i]=(struct pte_directory *)malloc(sizeof(struct pte_directory));

			for(int j=0;j<16;j++){
				if(current->pagetable.outer_ptes[i]->ptes[j].valid==false)
					continue;

				else { // 쓰기 모드 해제
					np->pagetable.outer_ptes[i]->ptes[j].valid=true;
					np->pagetable.outer_ptes[i]->ptes[j].writable=false;	
					np->pagetable.outer_ptes[i]->ptes[j].pfn=current->pagetable.outer_ptes[i]->ptes[j].pfn;	
					np->pagetable.outer_ptes[i]->ptes[j].private=current->pagetable.outer_ptes[i]->ptes[j].private;
			
					current->pagetable.outer_ptes[i]->ptes[j].writable=false;

					if(current->pagetable.outer_ptes[i]->ptes[j].valid==true) // valid가 유효하다면 pfn값 늘리기
						mapcounts[current->pagetable.outer_ptes[i]->ptes[j].pfn]++;
				}
		}
		}
	}
	list_add_tail(&current->list,&processes); // 

	current = np;
	ptbr = &np->pagetable;
	
}

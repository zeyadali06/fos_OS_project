/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include "trap.h"
#include <kern/proc/user_environment.h>
#include "../cpu/sched.h"
#include "../disk/pagefile_manager.h"
#include "../mem/memory_manager.h"

// 2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
//  0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;

//===============================
// REPLACEMENT STRATEGIES
//===============================
// 2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE)
{
	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE;
}
void setPageReplacmentAlgorithmCLOCK() { _PageRepAlgoType = PG_REP_CLOCK; }
void setPageReplacmentAlgorithmFIFO() { _PageRepAlgoType = PG_REP_FIFO; }
void setPageReplacmentAlgorithmModifiedCLOCK() { _PageRepAlgoType = PG_REP_MODIFIEDCLOCK; }
/*2018*/ void setPageReplacmentAlgorithmDynamicLocal() { _PageRepAlgoType = PG_REP_DYNAMIC_LOCAL; }
/*2021*/ void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps)
{
	_PageRepAlgoType = PG_REP_NchanceCLOCK;
	page_WS_max_sweeps = PageWSMaxSweeps;
}

// 2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE) { return _PageRepAlgoType == LRU_TYPE ? 1 : 0; }
uint32 isPageReplacmentAlgorithmCLOCK()
{
	if (_PageRepAlgoType == PG_REP_CLOCK)
		return 1;
	return 0;
}
uint32 isPageReplacmentAlgorithmFIFO()
{
	if (_PageRepAlgoType == PG_REP_FIFO)
		return 1;
	return 0;
}
uint32 isPageReplacmentAlgorithmModifiedCLOCK()
{
	if (_PageRepAlgoType == PG_REP_MODIFIEDCLOCK)
		return 1;
	return 0;
}
/*2018*/ uint32 isPageReplacmentAlgorithmDynamicLocal()
{
	if (_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL)
		return 1;
	return 0;
}
/*2021*/ uint32 isPageReplacmentAlgorithmNchanceCLOCK()
{
	if (_PageRepAlgoType == PG_REP_NchanceCLOCK)
		return 1;
	return 0;
}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt) { _EnableModifiedBuffer = enableIt; }
uint8 isModifiedBufferEnabled() { return _EnableModifiedBuffer; }

void enableBuffering(uint32 enableIt) { _EnableBuffering = enableIt; }
uint8 isBufferingEnabled() { return _EnableBuffering; }

void setModifiedBufferLength(uint32 length) { _ModifiedBufferLength = length; }
uint32 getModifiedBufferLength() { return _ModifiedBufferLength; }

//===============================
// FAULT HANDLERS
//===============================

// Handle the table fault
void table_fault_handler(struct Env *curenv, uint32 fault_va)
{
	// panic("table_fault_handler() is not implemented yet...!!");
	// Check if it's a stack page
	uint32 *ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory, (uint32)fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}

// Handle the page fault

void page_fault_handler(struct Env *curenv, uint32 fault_va)
{
#if USE_KHEAP
	struct WorkingSetElement *victimWSElement = NULL;
	uint32 wsSize = LIST_SIZE(&(curenv->page_WS_list));
#else
	int iWS = curenv->page_last_WS_index;
	uint32 wsSize = env_page_ws_get_size(curenv);
#endif

	cprintf("Enter\n");
	if (wsSize < (curenv->page_WS_max_size))
	{
		// cprintf("PLACEMENT=========================WS Size = %d\n", wsSize );
		// TODO: [PROJECT'23.MS2 - #15] [3] PAGE FAULT HANDLER - Placement
		//  Write your code here, remove the panic and write your code
		// panic("page_fault_handler().PLACEMENT is not implemented yet...!!");

		cprintf("%d\n", pf_calculate_allocated_pages(curenv));
		struct WorkingSetElement *ele = env_page_ws_list_create_element(curenv, fault_va);
		struct FrameInfo *frame_info_ptr;

		if (allocate_frame(&frame_info_ptr) == 0)
		{
			map_frame(curenv->env_page_directory, frame_info_ptr, fault_va, PERM_USER | PERM_WRITEABLE);
			frame_info_ptr->va = fault_va;
		}

		if (pf_read_env_page(curenv, (void *)fault_va) == E_PAGE_NOT_EXIST_IN_PF)
		{
			// cprintf("%x %x %x %x %x\n", fault_va, USTACKTOP, USTACKBOTTOM, USER_HEAP_MAX, USER_HEAP_START);

			if ((fault_va <= USTACKTOP && fault_va >= USTACKBOTTOM) || (fault_va <= USER_HEAP_MAX && fault_va >= USER_HEAP_START))
			{
				// cprintf("USTACKTOP\n");
				// if (pf_add_empty_env_page(curenv, fault_va, 0) == E_NO_PAGE_FILE_SPACE)
				// 	panic("ERROR: No enough virtual space on the page file");
				// pf_add_empty_env_page(curenv, fault_va, 0);
				LIST_INSERT_TAIL(&(curenv->page_WS_list), ele);
				return;
			}
			else
			{
				// cprintf("Kill\n");
				sched_kill_env(curenv->env_id);
				return;
			}
		}

		LIST_INSERT_TAIL(&(curenv->page_WS_list), ele);
		// pf_update_env_page(curenv, fault_va, get_frame_info(curenv->env_page_directory, fault_va, NULL));

		// cprintf("%d\n", pf_calculate_allocated_pages(curenv));

		// cprintf("Quit\n");

		// int size = curenv->page_WS_max_size;
		// for (int i = 0; i < size; i++)
		// {
		// 	if (curenv->__ptr_tws[curenv->page_last_WS_index].empty)
		// 		break;
		// 	else if (curenv->__ptr_tws[i].empty)
		// 	{
		// 		curenv->page_last_WS_index = i;
		// 		break;
		// 	}
		// }

		// pf_update_env_page(curenv, fault_va, get_frame_info(curenv->env_page_directory, fault_va, NULL));

				// refer to the project presentation and documentation for details
	}
	else
	{
		// cprintf("REPLACEMENT=========================WS Size = %d\n", wsSize );
		// refer to the project presentation and documentation for details
		if (isPageReplacmentAlgorithmFIFO())
		{
			// TODO: [PROJECT'23.MS3 - #1] [1] PAGE FAULT HANDLER - FIFO Replacement
			//  Write your code here, remove the panic and write your code
			panic("page_fault_handler() FIFO Replacement is not implemented yet...!!");
		}
		if (isPageReplacmentAlgorithmLRU(PG_REP_LRU_LISTS_APPROX))
		{
			// TODO: [PROJECT'23.MS3 - #2] [1] PAGE FAULT HANDLER - LRU Replacement
			//  Write your code here, remove the panic and write your code
			panic("page_fault_handler() LRU Replacement is not implemented yet...!!");

			// TODO: [PROJECT'23.MS3 - BONUS] [1] PAGE FAULT HANDLER - O(1) implementation of LRU replacement
		}
	}
}

void __page_fault_handler_with_buffering(struct Env *curenv, uint32 fault_va)
{
	panic("this function is not required...!!");
}

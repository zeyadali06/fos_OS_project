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

// void print_data(struct WS_List list)
// {
// 	struct WorkingSetElement *curr;
// 	LIST_FOREACH(curr, &list)
// 	{
// 		cprintf("virtual address: %x, sweeps_counter: %d, time_stamp: %d, empty: %d\n", curr->virtual_address, curr->sweeps_counter, curr->time_stamp, curr->empty);
// 	}
// }

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

	// cprintf("s\n");
	if (isPageReplacmentAlgorithmFIFO())
	{

		// cprintf("fifo");
		if (wsSize < (curenv->page_WS_max_size))
		{
			// cprintf("1");
			// cprintf("PLACEMENT=========================WS Size = %d\n", wsSize );
			// TODO: [PROJECT'23.MS2 - #15] [3] PAGE FAULT HANDLER - Placement
			//  Write your code here, remove the panic and write your code
			// panic("page_fault_handler().PLACEMENT is not implemented yet...!!");

			// cprintf("sizebefore:%d\n", pf_calculate_allocated_pages(curenv));

			struct WorkingSetElement *ele = env_page_ws_list_create_element(curenv, fault_va);
			struct FrameInfo *frame_info_ptr;

			if (allocate_frame(&frame_info_ptr) == 0)
			{
				// cprintf("2\n");
				map_frame(curenv->env_page_directory, frame_info_ptr, fault_va, PERM_MARKED | PERM_USER | PERM_WRITEABLE);
				frame_info_ptr->va = fault_va;
			}

			if (pf_read_env_page(curenv, (void *)fault_va) == E_PAGE_NOT_EXIST_IN_PF)
			{
				// cprintf("fault_va: %x  %x  %x  %x  %x  \n", fault_va,USTACKBOTTOM,USTACKTOP, USER_HEAP_START,USER_HEAP_MAX);

				if ((fault_va <= USTACKTOP && fault_va >= USTACKBOTTOM) || (fault_va <= USER_HEAP_MAX && fault_va >= USER_HEAP_START))
				{
					// cprintf("4");
					// cprintf("USTACKTOP\n");
					// if (pf_add_empty_env_page(curenv, fault_va, 0) == E_NO_PAGE_FILE_SPACE)
					// 	panic("ERROR: No enough virtual space on the page file");

					LIST_INSERT_TAIL(&(curenv->page_WS_list), ele);
					if (curenv->page_WS_max_size == (LIST_SIZE(&(curenv->page_WS_list))))
					{
						// cprintf("5");
						curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
					}
					// pf_update_env_page(curenv, fault_va, get_frame_info(curenv->env_page_directory, 0, NULL));
					// pf_add_empty_env_page(curenv, fault_va, 1);

					// cprintf("sizeafter:%d, maxSize:%d\n", LIST_SIZE(&(curenv->page_WS_list)), curenv->page_WS_max_size);
				}
				else
				{
					// cprintf("6");
					// cprintf("Kill\n");
					sched_kill_env(curenv->env_id);
					return;
				}
			}
			else
			{
				LIST_INSERT_TAIL(&(curenv->page_WS_list), ele);
				if (curenv->page_WS_max_size == (LIST_SIZE(&(curenv->page_WS_list))))
				{
					// cprintf("5");
					curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
				}
			}

			// pf_update_env_page(curenv, fault_va, get_frame_info(curenv->env_page_directory, fault_va, NULL));

			// refer to the project presentation and documentation for details
		}
		else
		{
			// cprintf("7");
			// cprintf("REPLACEMENT=========================WS Size = %d\n", wsSize );
			// refer to the project presentation and documentation for details

			// TODO: [PROJECT'23.MS3 - #1] [1] PAGE FAULT HANDLER - FIFO Replacement
			//  Write your code here, remove the panic and write your code
			// panic("page_fault_handler() FIFO Replacement is not implemented yet...!!");

			// cprintf("Enter Replacment\n");

			uint32 *ptrPage;
			uint32 deleted;
			deleted = curenv->page_last_WS_element->virtual_address;
			struct WorkingSetElement *ele = env_page_ws_list_create_element(curenv, fault_va);

			struct FrameInfo *ptrFrame;
			if (allocate_frame(&ptrFrame) == 0)
			{
				map_frame(curenv->env_page_directory, ptrFrame, fault_va, PERM_USER | PERM_MARKED | PERM_WRITEABLE);
				ptrFrame->va = fault_va;
				ptrFrame->element = ele;
				int ret = pf_read_env_page(curenv, (void *)fault_va);
			}

			struct FrameInfo *deletedFrame = get_frame_info(curenv->env_page_directory, deleted, &ptrPage);

			if (LIST_NEXT(curenv->page_last_WS_element) != NULL)
			{
				// cprintf("Enter != NULL\n");
				curenv->page_last_WS_element = LIST_NEXT(curenv->page_last_WS_element);
				struct WorkingSetElement *e = LIST_PREV(curenv->page_last_WS_element);
				LIST_REMOVE(&(curenv->page_WS_list), e);
				// LIST_INSERT_TAIL(&(curenv->page_WS_list), ele);
				LIST_INSERT_BEFORE(&(curenv->page_WS_list), curenv->page_last_WS_element, ele);
				// cprintf("Quit != NULL\n");
			}
			else
			{
				// cprintf("Enter == NULL\n");
				curenv->page_last_WS_element = LIST_PREV(curenv->page_last_WS_element);
				struct WorkingSetElement *e = LIST_NEXT(curenv->page_last_WS_element);
				LIST_REMOVE(&(curenv->page_WS_list), e);
				// LIST_INSERT_TAIL(&(curenv->page_WS_list), ele);
				LIST_INSERT_AFTER(&(curenv->page_WS_list), curenv->page_last_WS_element, ele);
				curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
				// cprintf("Quit == NULL\n");
			}

			int perms = pt_get_page_permissions(curenv->env_page_directory, deleted);
			if (perms & PERM_MODIFIED)
			{
				pf_update_env_page(curenv, deleted, deletedFrame);
			}

			unmap_frame(curenv->env_page_directory, deleted);

			// cprintf("deleted frame: %x, deleted: %x, page table: %x, entry: %x\n", deletedFrame->va, deleted, ptrPage, ptrPage[PTX(fault_va)]);

			// env_page_ws_print(curenv);

			// cprintf("--------------------------------------------------\n");
		}
	}

	if (isPageReplacmentAlgorithmLRU(PG_REP_LRU_LISTS_APPROX))
	{
		// fault_va &= 0xFFFFF000;
		// cprintf("--------------------------------------------------\n");
		// cprintf("va: %x\n", fault_va);
		uint32 activeListSize = LIST_SIZE(&(curenv->ActiveList));
		uint32 secondListSize = LIST_SIZE(&(curenv->SecondList));

		if ((activeListSize + secondListSize) < curenv->page_WS_max_size)
		{
			// cprintf("Placment va: %x\n", fault_va);
			struct FrameInfo *frame_info_ptr;

			if (activeListSize < curenv->ActiveListSize)
			{
				// cprintf("active not full\n");
				if (allocate_frame(&frame_info_ptr) == 0)
				{
					map_frame(curenv->env_page_directory, frame_info_ptr, fault_va, PERM_MARKED | PERM_USER | PERM_WRITEABLE);
					frame_info_ptr->va = fault_va;
					pf_read_env_page(curenv, (void *)fault_va);
				}
				struct WorkingSetElement *ele = env_page_ws_list_create_element(curenv, fault_va);
				LIST_INSERT_HEAD(&(curenv->ActiveList), ele);

				// env_page_ws_print(curenv);
				// cprintf("--------------------------------------------------\n");
				return;
			}
			else
			{
				struct WorkingSetElement *currele;
				LIST_FOREACH(currele, &(curenv->SecondList))
				{
					if ((uint32)(currele->virtual_address & 0xFFFFF000) == (uint32)(fault_va & 0xFFFFF000))
					{
						// cprintf("insertion\n");
						struct WorkingSetElement *ele = env_page_ws_list_create_element(curenv, fault_va);

						struct WorkingSetElement *firstListLastEle1 = LIST_LAST(&(curenv->ActiveList));
						LIST_REMOVE(&(curenv->ActiveList), firstListLastEle1);
						pt_set_page_permissions(curenv->env_page_directory, firstListLastEle1->virtual_address, 0, PERM_PRESENT);
						LIST_INSERT_HEAD(&(curenv->SecondList), firstListLastEle1);

						struct WorkingSetElement *currele1 = currele;
						LIST_REMOVE(&(curenv->SecondList), currele1);
						pt_set_page_permissions(curenv->env_page_directory, ele->virtual_address, PERM_PRESENT, 0);
						LIST_INSERT_HEAD(&(curenv->ActiveList), ele);

						// env_page_ws_print(curenv);
						// cprintf("--------------------------------------------------\n");

						return;
					}
				}
				if (allocate_frame(&frame_info_ptr) == 0)
				{
					map_frame(curenv->env_page_directory, frame_info_ptr, fault_va, PERM_MARKED | PERM_USER | PERM_WRITEABLE);
					frame_info_ptr->va = fault_va;
					pf_read_env_page(curenv, (void *)fault_va);
				}
				// cprintf("insertion faild\n");
				struct WorkingSetElement *firstListLastEle1 = LIST_LAST(&(curenv->ActiveList));

				LIST_REMOVE(&(curenv->ActiveList), firstListLastEle1);
				pt_set_page_permissions(curenv->env_page_directory, firstListLastEle1->virtual_address, 0, PERM_PRESENT);
				// uint32 *ptrPage;
				// unmap_frame(curenv->env_page_directory, firstListLastEle1->virtual_address);
				LIST_INSERT_HEAD(&(curenv->SecondList), firstListLastEle1);

				struct WorkingSetElement *ele = env_page_ws_list_create_element(curenv, fault_va);
				pt_set_page_permissions(curenv->env_page_directory, ele->virtual_address, PERM_PRESENT, 0);
				LIST_INSERT_HEAD(&(curenv->ActiveList), ele);

				// env_page_ws_print(curenv);
				// cprintf("--------------------------------------------------\n");
				return;
			}
		}
		else
		{
			// TODO: [PROJECT'23.MS3 - #2] [1] PAGE FAULT HANDLER - LRU Replacement
			//  Write your code here, remove the panic and write your code

			// cprintf("------------------------------------------------------------\n");
			// cprintf("Replacment va: %x\n", fault_va);

			// cprintf("Replacment\n");
			struct WorkingSetElement *ele = env_page_ws_list_create_element(curenv, fault_va);
			struct FrameInfo *frame_info_ptr;
			if (allocate_frame(&frame_info_ptr) == 0)
			{
				map_frame(curenv->env_page_directory, frame_info_ptr, fault_va, PERM_MARKED | PERM_USER | PERM_WRITEABLE);
				frame_info_ptr->va = fault_va;
				frame_info_ptr->element = ele;
				int ret = pf_read_env_page(curenv, (void *)fault_va);
				// pt_set_page_permissions(curenv->env_page_directory, fault_va,PERM_PRESENT , 0);
			}

			struct WorkingSetElement *currele;
			LIST_FOREACH(currele, &(curenv->SecondList))
			{
				if ((uint32)(currele->virtual_address & 0xFFFFF000) == (uint32)(fault_va & 0xFFFFF000))
				{

					// cprintf("insertion\n");
					// struct WorkingSetElement *ele = env_page_ws_list_create_element(curenv, fault_va);

					struct WorkingSetElement *firstListLastEle1 = LIST_LAST(&(curenv->ActiveList));
					LIST_REMOVE(&(curenv->ActiveList), firstListLastEle1);
					pt_set_page_permissions(curenv->env_page_directory, firstListLastEle1->virtual_address, 0, PERM_PRESENT);
					LIST_INSERT_HEAD(&(curenv->SecondList), firstListLastEle1);

					struct WorkingSetElement *currele1 = currele;
					LIST_REMOVE(&(curenv->SecondList), currele1);
					pt_set_page_permissions(curenv->env_page_directory, ele->virtual_address, PERM_PRESENT, 0);
					LIST_INSERT_HEAD(&(curenv->ActiveList), ele);

					// cprintf("Finish loop\n");
					// env_page_ws_print(curenv);
					// cprintf("--------------------------------------------------\n");

					return;
				}
			}

			struct WorkingSetElement *secondListLastEle = LIST_LAST(&(curenv->SecondList));
			int perms = pt_get_page_permissions(curenv->env_page_directory, secondListLastEle->virtual_address);
			if (perms & PERM_MODIFIED)
			{
				uint32 *ptrPage;
				struct FrameInfo *frame = get_frame_info(curenv->env_page_directory, secondListLastEle->virtual_address, &ptrPage);
				pf_update_env_page(curenv, secondListLastEle->virtual_address, frame);
			}
			pt_set_page_permissions(curenv->env_page_directory, secondListLastEle->virtual_address, 0, PERM_PRESENT);
			LIST_REMOVE(&(curenv->SecondList), secondListLastEle);
			unmap_frame(curenv->env_page_directory, secondListLastEle->virtual_address);

			struct WorkingSetElement *firstListLastEle1 = LIST_LAST(&(curenv->ActiveList));
			LIST_REMOVE(&(curenv->ActiveList), firstListLastEle1);
			pt_set_page_permissions(curenv->env_page_directory, firstListLastEle1->virtual_address, 0, PERM_PRESENT);
			LIST_INSERT_HEAD(&(curenv->SecondList), firstListLastEle1);

			// pt_set_page_permissions(curenv->env_page_directory, ele->virtual_address, PERM_PRESENT, 0);
			LIST_INSERT_HEAD(&(curenv->ActiveList), ele);

			// cprintf("Finish\n");
			// env_page_ws_print(curenv);
			// cprintf("------------------------------------------------------------\n");

			// TODO: [PROJECT'23.MS3 - BONUS] [1] PAGE FAULT HANDLER - O(1) implementation of LRU replacement
		}
	}
}

void __page_fault_handler_with_buffering(struct Env *curenv, uint32 fault_va)
{
	panic("this function is not required...!!");
}

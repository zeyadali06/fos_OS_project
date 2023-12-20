#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"

int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit)
{
	// TODO: [PROJECT'23.MS2 - #01] [1] KERNEL HEAP - initialize_kheap_dynamic_allocator()
	// Initialize the dynamic allocator of kernel heap with the given start address, size & limit
	// All pages in the given range should be allocated and mapped
	// Remember: call the initialize_dynamic_allocator(..) to complete the initialization
	// Return:
	//	On success: 0
	//	Otherwise (if no memory OR initial size exceed the given limit): E_NO_MEM

	if (daStart + initSizeToAllocate > daLimit)
		return E_NO_MEM;

	startOfKernalHeap = daStart;
	brk = daStart + initSizeToAllocate;
	rlimit = daLimit;
	// rlimit = 0xF8000000

	uint32 virtual_address = daStart;

	// for (int i = 0; i < ROUNDUP((rlimit - daStart), PAGE_SIZE) / PAGE_SIZE; i++)
	for (int i = 0; i < ROUNDUP((initSizeToAllocate), PAGE_SIZE) / PAGE_SIZE; i++)
	{
		struct FrameInfo *ptr;
		if (allocate_frame(&ptr) == 0)
		{
			map_frame(ptr_page_directory, ptr, virtual_address, PERM_WRITEABLE | PERM_USED);
			ptr->va = virtual_address & 0xFFFFF000;
		}
		else
		{
			return E_NO_MEM;
		}
		virtual_address += PAGE_SIZE;
	}

	initialize_dynamic_allocator(daStart, initSizeToAllocate);

	// Comment the following line(s) before start coding...
	// panic("not implemented yet");
	return 0;
}

void *sbrk(int increment)
{
	// TODO: [PROJECT'23.MS2 - #02] [1] KERNEL HEAP - sbrk()
	/* increment > 0: move the segment break of the kernel to increase the size of its heap,
	 * 				you should allocate pages and map them into the kernel virtual address space as necessary,
	 * 				and returns the address of the previous break (i.e. the beginning of newly mapped memory).
	 * increment = 0: just return the current position of the segment break
	 * increment < 0: move the segment break of the kernel to decrease the size of its heap,
	 * 				you should deallocate pages that no longer contain part of the heap as necessary.
	 * 				and returns the address of the new break (i.e. the end of the current heap space).
	 *
	 * NOTES:
	 * 	1) You should only have to allocate or deallocate pages if the segment break crosses a page boundary
	 * 	2) New segment break should be aligned on page-boundary to avoid "No Man's Land" problem
	 * 	3) Allocating additional pages for a kernel dynamic allocator will fail if the free frames are exhausted
	 * 		or the break exceed the limit of the dynamic allocator. If sbrk fails, kernel should panic(...)
	 */

	if (increment == 0)
		return (void *)brk;

	// cprintf("size: %d, frames: %d\n", increment, calculate_available_frames().freeBuffered + calculate_available_frames().freeNotBuffered);
	if (increment > 0)
	{
		if (ROUNDUP(brk + increment, PAGE_SIZE) <= rlimit)
		{
			uint32 prevBrk = brk;
			uint32 oldBRK = brk + PAGE_SIZE;

			if (PTX(brk + increment) != PTX(brk - 1))
			{
				bool ch = 0;
				// cprintf("pages: %d\n", ROUNDUP(increment, PAGE_SIZE) / PAGE_SIZE);
				uint32 *ptrPage;
				if (brk % PAGE_SIZE == 0 && get_frame_info(ptr_page_directory, brk, &ptrPage) == 0)
				{
					// cprintf("ok\n");
					ch = 1;
					struct FrameInfo *ptr_frame_info;
					if (allocate_frame(&ptr_frame_info) == 0)
					{
						// cprintf("%x\n", brk & 0xFFFFF000);
						map_frame(ptr_page_directory, ptr_frame_info, brk, PERM_WRITEABLE | PERM_USED);
						ptr_frame_info->va = brk;
					}
				}

				int size = ROUNDUP(increment, PAGE_SIZE) / PAGE_SIZE;
				if (ch)
				{
					size--;
				}

				for (int i = 0; i < size; i++)
				{
					// if (get_frame_info(ptr_page_directory, oldBRK, &ptrPage) == 0)
					// {
					struct FrameInfo *ptr_frame_info;
					if (allocate_frame(&ptr_frame_info) == 0)
					{
						// cprintf("%x\n", oldBRK & 0xFFFFF000);
						map_frame(ptr_page_directory, ptr_frame_info, oldBRK, PERM_WRITEABLE | PERM_USED);
						// ptr_frame_info->references = 1;
						ptr_frame_info->va = oldBRK;
					}
					oldBRK += PAGE_SIZE;
					// }
				}
			}

			brk = ROUNDUP(brk + increment, PAGE_SIZE);

			// cprintf("Quit increment %d\n", calculate_available_frames().freeBuffered + calculate_available_frames().freeNotBuffered);

			return (void *)prevBrk;
		}
		else
		{
			panic("Not enough memory");
		}
	}

	if (increment < 0)
	{
		increment *= -1;
		if (brk - increment >= startOfKernalHeap)
		{
			// cprintf("Enter va %x %x %x %x\n", brk, ROUNDDOWN(brk, PAGE_SIZE), ROUNDDOWN(brk - increment, PAGE_SIZE), startOfKernalHeap);
			uint32 *ptrPageTable;

			if (PTX(brk - increment) != PTX(brk - 1))
			{
				uint32 oldBRK = brk - 1;

				// cprintf("pages: %d\n", ROUNDUP(increment, PAGE_SIZE) / PAGE_SIZE);

				for (int i = 0; i < ROUNDUP(increment, PAGE_SIZE) / PAGE_SIZE; i++)
				{
					// cprintf("%x\n", oldBRK & 0xFFFFF000);
					unmap_frame(ptr_page_directory, oldBRK);
					// env_page_ws_invalidate(env, oldBRK);
					oldBRK -= PAGE_SIZE;
					if (oldBRK - (brk - increment) < PAGE_SIZE)
						break;
				}
			}

			brk -= increment;

			// cprintf("Quit decrement %d\n", calculate_available_frames().freeBuffered + calculate_available_frames().freeNotBuffered);

			return (void *)brk;
		}
	}

	// MS2: COMMENT THIS LINE BEFORE START CODING====
	return (void *)-1;
	// panic("not implemented yet");
}

void *kmalloc(unsigned int size)
{
	// TODO: [PROJECT'23.MS2 - #03] [1] KERNEL HEAP - kmalloc()
	// refer to the project presentation and documentation for details
	//  use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy
	// 	uint32 tset;
	// 	for (int i = 0; i < 1024; i++)
	// 	{
	// (uint32)ptr_page_directory
	// 	}
	// cprintf("----------------------------------------------------------------\n");
	// cprintf("size: %d %d %d\n", size, ROUNDUP(size, PAGE_SIZE), ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE);
	// cprintf("Enter kmalloc\n");
	if (size >= (KERNEL_HEAP_MAX - ((uint32)rlimit + 4096)) || size >= ((uint32)rlimit - KERNEL_HEAP_START))
	{
		// cprintf("----------------------------------------------------------------\n");
		return NULL;
	}

	if (size <= DYN_ALLOC_MAX_BLOCK_SIZE)
	{
		// cprintf("----------------------------------------------------------------\n");
		return alloc_block(size, DA_FF);
	}

	// cprintf("Enter malloc size: %d\n", ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE);

	uint32 va = (uint32)rlimit + PAGE_SIZE;
	// cprintf("va: %x\n", va);
	for (int i = 0; i < KArrSize; i++)
	{
		uint32 *ptrPage;
		if (get_page_table(ptr_page_directory, (uint32)va, &ptrPage) == TABLE_IN_MEMORY)
		{
			// cprintf("Page table exist %d\n", i);
			// cprintf("%x %x %x\n", ptrPage, PTX(va), (uint32)(ptrPage[PTX(va)]));
			// cprintf("page entry: %x\n", (uint32)(ptrPage[PTX(va)]));

			if ((uint32)((ptrPage[PTX(va)]) & 0xFFFFF000) == 0)
			{
				bool enoughFreeSpace = 1;
				uint32 checkableVA = va;
				uint32 returnedVA = va;
				for (int l = 0; l < (ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE) - 1; l++)
				{
					// cprintf("%x\n", (uint32)(ptrPage[PTX(checkableVA)]));
					if ((uint32)(ptrPage[PTX(checkableVA)]) != 0)
					{
						// checkableVA += PAGE_SIZE;
						enoughFreeSpace = 0;
						// cprintf("Exit\n");
						break;
					}

					if (PTX(checkableVA) == 1023)
					{
						// checkableVA += PAGE_SIZE;
						get_page_table(ptr_page_directory, (checkableVA + PAGE_SIZE), &ptrPage);
						// if (get_page_table(ptr_page_directory, (checkableVA + PAGE_SIZE), &ptrPage) == TABLE_NOT_EXIST)
						// {
						// 	cprintf("--------------------------------------------------------\n");
						// 	ptrPage = create_page_table(ptr_page_directory, (checkableVA + PAGE_SIZE));
						// }
					}

					checkableVA += PAGE_SIZE;
				}

				if (enoughFreeSpace == 0)
				{
					va += PAGE_SIZE;
					continue;
				}

				for (int l = 0; l < (ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE); l++)
				{
					// cprintf("Enter\n");
					struct FrameInfo *ptr_frame_info;
					if (allocate_frame(&ptr_frame_info) == 0)
					{
						map_frame(ptr_page_directory, ptr_frame_info, va, PERM_WRITEABLE | PERM_USED);
						ptr_frame_info->va = va & 0xFFFFF000;
					}
					else
						return NULL;

					va += PAGE_SIZE;
				}

				for (int i = 0; i < KArrSize; i++)
				{
					if (addresses[i].va == 0 && addresses[i].size == 0)
					{
						addresses[i].va = returnedVA & 0xFFFFF000;
						addresses[i].size = size;
						break;
					}
				}

				// cprintf("VA: %x\n", returnedVA);

				// cprintf("Free Entry Loaded Succesfully %d %x %x\n", i, returnedVA, va);
				// cprintf("----------------------------------------------------------------\n");
				return (void *)returnedVA;
			}
			else
				va += PAGE_SIZE;
		}

		if (va > KERNEL_HEAP_MAX)
			return NULL;
	}

	// change this "return" according to your answer
	// kpanic_into_prompt("kmalloc() is not implemented yet...!!");
	// cprintf("----------------------------------------------------------------\n");
	// cprintf("Quit malloc size: %d\n", ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE);

	return NULL;
}

void kfree(void *virtual_address)
{
	// TODO: [PROJECT'23.MS2 - #04] [1] KERNEL HEAP - kfree()
	// refer to the project presentation and documentation for details
	//  Write your code here, remove the panic and write your code
	// cprintf("Enter kfree\n");
	if ((uint32)virtual_address >= KERNEL_HEAP_START && (uint32)virtual_address <= brk)
	{
		free_block(virtual_address);
		// cprintf("----------------------------------------------------------------\n");
		return;
	}

	if ((uint32)virtual_address < rlimit + 4096 || (uint32)virtual_address > KERNEL_HEAP_MAX)
	{
		panic("Invalid Address");
		// return;
	}

	// cprintf("Enter\n");

	uint32 size = 0;
	for (int i = 0; i < KArrSize; i++)
	{
		// cprintf("%x %x\n", virtual_address, addresses[i].va);
		if ((addresses[i].va & 0xFFFFF000) == ((uint32)virtual_address & 0xFFFFF000))
		{
			// cprintf("free   NOPages coresponding to VA: %d  VA: %x\n", (ROUNDUP(addresses[i].size, PAGE_SIZE) / PAGE_SIZE), addresses[i].va);
			// va = addresses[i].va;
			size = addresses[i].size;
			addresses[i].size = 0;
			addresses[i].va = 0;
			break;
		}
		// if (((addresses[i] == addresses[i + 1]) || (addresses[i] == addresses[i - 1])) && addresses[i] == (uint32 *)virtual_address)
		// 	// if (((addresses[i] == addresses[i + 1]) ) && addresses[i] == (uint32 *)virtual_address)
		// 	numOfPages++;
	}

	if (size != 0)
	{
		for (int i = 0; i < (ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE); i++)
		{
			// cprintf("%d\n", free_frame_list.size);
			// struct FrameInfo *fptr;
			// uint32 *ptrPageTable;
			// fptr = get_frame_info(ptr_page_directory, (uint32)virtual_address, &ptrPageTable);
			// fptr->va = 0;
			// free_frame(fptr);
			unmap_frame(ptr_page_directory, (uint32)virtual_address);
			virtual_address += PAGE_SIZE;
			// cprintf("Finish UnMapping\n");
		}
	}
	// cprintf("----------------------------------------------------------------\n");

	// cprintf("Quit\n");

	// panic("kfree() is not implemented yet...!!");
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	// TODO: [PROJECT'23.MS2 - #05] [1] KERNEL HEAP - kheap_virtual_address()
	// refer to the project presentation and documentation for details
	//  Write your code here, remove the panic and write your code
	// panic("kheap_virtual_address() is not implemented yet...!!");

	// EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================

	struct FrameInfo *ptr_frame_info;
	ptr_frame_info = to_frame_info((uint32)physical_address & 0xFFFFF000);
	if (ptr_frame_info->references == 0)
	{
		return 0;
	}
	uint32 va = ptr_frame_info->va;
	uint32 offset = physical_address - ROUNDDOWN(physical_address, PAGE_SIZE);
	// cprintf("va = %x, offset = %x, PA = %x,", va, offset, physical_address);
	return va + offset;

	// change this "return" according to your answer
	// return 0;
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{

	uint32 *ptr_page_table = NULL;
	get_page_table(ptr_page_directory, virtual_address, &ptr_page_table);
	if (ptr_page_table == NULL)
		return 0;

	uint32 offset = ((uint32)virtual_address & 0xfff);

	return (unsigned int)((ptr_page_table[PTX(virtual_address)] & 0xFFFFF000) | offset);

	// TODO: [PROJECT'23.MS2 - #06] [1] KERNEL HEAP - kheap_physical_address()
	// refer to the project presentation and documentation for details
	//  Write your code here, remove the panic and write your code
	// panic("kheap_physical_address() is not implemented yet...!!");
	// change this "return" according to your answer
	// return 0;
}

void kfreeall()
{
	panic("Not implemented!");
}

void kshrink(uint32 newSize)
{
	panic("Not implemented!");
}

void kexpand(uint32 newSize)
{
	panic("Not implemented!");
}

//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	// TODO: [PROJECT'23.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc()
	//  Write your code here, remove the panic and write your code
	return NULL;
	panic("krealloc() is not implemented yet...!!");
}

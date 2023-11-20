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

	startOfKernalHeap = (uint32 *)daStart;
	brk = (uint32 *)(daStart + initSizeToAllocate);
	rlimit = (uint32 *)daLimit;

	uint32 virtual_address = daStart;

	for (int i = 0; i < ROUNDUP(initSizeToAllocate, PAGE_SIZE) / PAGE_SIZE; i++)
	{
		struct FrameInfo *ptr;
		if (allocate_frame(&ptr) == 0)
		{
			map_frame(ptr_page_directory, ptr, (uint32)virtual_address, PERM_WRITEABLE);
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

	if (increment > 0)
	{
		if (brk + ROUNDUP(increment, PAGE_SIZE) < rlimit && (ROUNDUP(increment, PAGE_SIZE) / PAGE_SIZE) >= free_frame_list.size)
		{
			uint32 prevBrk = (uint32)brk;
			for (int i = 0; i < ROUNDUP(increment, PAGE_SIZE) / PAGE_SIZE; i++)
			{
				struct FrameInfo *ptr;
				allocate_frame(&ptr);
				map_frame(ptr_page_directory, ptr, (uint32)brk, PERM_WRITEABLE);
				// cprintf("%d %d\n", allocate_frame(&ptr), map_frame(ptr_page_directory, ptr, (uint32)virtual_address, PERM_WRITEABLE));
				brk += PAGE_SIZE;
			}
			return (void *)prevBrk;
		}
		else
		{
			panic("Not enough memory");
		}
	}

	if (increment < 0)
	{
		uint32 prevBrk = (uint32)brk;
		increment *= -1;
		for (int i = 0; i < ROUNDDOWN(increment, PAGE_SIZE) / PAGE_SIZE; i++)
		{
			struct FrameInfo *ptr;
			uint32 *ptrPageTable;
			unmap_frame(ptr_page_directory, (uint32)brk);
			ptr = get_frame_info(ptr_page_directory, (uint32)brk, &ptrPageTable);
			free_frame(ptr);

			brk -= PAGE_SIZE;
		}
		brk = (uint32 *)(prevBrk - increment);
		return (void *)brk;
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

	if (size >= (KERNEL_HEAP_MAX - ((uint32)rlimit + 4096)) || size >= ((uint32)rlimit - KERNEL_HEAP_START))
	{
		return NULL;
	}

	if (size <= DYN_ALLOC_MAX_BLOCK_SIZE)
	{
		return alloc_block(size, DA_FF);
	}

	uint32 va = (uint32)rlimit + PAGE_SIZE;

	for (int i = 0; i < (1024 * 1024); i++)
	{
		uint32 *ptrPage;
		if (get_page_table(ptr_page_directory, (uint32)va, &ptrPage) == TABLE_IN_MEMORY)
		{
			// cprintf("Page table exist %d\n", i);
			cprintf("%x %x %x\n", ptrPage, PTX(va), *((uint32 *)((uint32)ptrPage + PTX(va))));

			if (*((uint32 *)((uint32)ptrPage + PTX(va))) == 0)
			{
				bool enoughFreeSpace = 1;
				uint32 checkableVA = va;
				uint32 returnedVA = va;
				for (int l = 0; l < ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE; l++)
				{
					if (*((uint32 *)((uint32)ptrPage + PTX(checkableVA))) != 0)
					{
						enoughFreeSpace = 0;
						break;
					}
					checkableVA += 0x1000;
				}
				if (!enoughFreeSpace)
				{
					va += 0x1000;
					continue;
				}
				for (int l = 0; l < ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE; l++)
				{
					cprintf("Enter\n");
					struct FrameInfo *ptr_frame_info;
					if (allocate_frame(&ptr_frame_info) == 0)
					{
						map_frame(ptr_page_directory, ptr_frame_info, va, PERM_WRITEABLE);
					}
					else
					{
						return NULL;
					}
					va += 0x1000;
				}
				cprintf("Free Entry Loaded Succesfully %d\n", i);
				return (void *)returnedVA;
			}
			else
			{
				va += 0x1000;
			}
		}
		else
		{
			cprintf("Page Table Not Exist %d\n", i);
			va += 0x1000;
		}
		// else
		// {
		// 	continue;
		// 	// cprintf("else\n");
		// 	for (int m = 0; m < ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE; m++)
		// 	{
		// 		struct FrameInfo *ptr_frame_info;
		// 		allocate_frame(&ptr_frame_info);
		// 		map_frame(ptr_page_directory, ptr_frame_info, (uint32)va, PERM_WRITEABLE);
		// 	}
		// 	return (void *)va;
		// 	// // cprintf("Page table not exist\n");
		// 	// ptrPage = create_page_table(ptr_page_directory, va);
		// 	// for (int r = 0; r < (ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE) - n; r++)
		// 	// {
		// 	// 	struct FrameInfo *ptr_frame_info;
		// 	// 	if (allocate_frame(&ptr_frame_info) == 0)
		// 	// 	{
		// 	// 		map_frame(ptr_page_directory, ptr_frame_info, va, PERM_WRITEABLE);
		// 	// 		// cprintf("Allocated Done Successfullly\n");
		// 	// 		break;
		// 	// 	}
		// 	// 	else
		// 	// 	{
		// 	// 		return NULL;
		// 	// 	}
		// 	// 	va += 0x1000;
		// 	// }
		// }
		if (va > KERNEL_HEAP_MAX)
		{
			cprintf("End Of Kernal Heap\n");
			break;
		}
		// cprintf("%x %d\n", ptrPage, i);
	}

	// for (int n = 0; n < ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE; n++)
	// {
	// 	uint32 *va = ptr_page_directory;
	// 	int ptrDirFN; // f# in an entry in the directory
	// 	cprintf("%x\n", va);
	// 	bool allocated = 0;
	// 	for (int i = 0; i < 1024; i++) // dir loop
	// 	{
	// 		uint32 *ptrPage;
	// 		int ptrPageFN; // f# in an entry in the page table
	// 		if (get_page_table(ptr_page_directory, (uint32)va, &ptrPage) == TABLE_IN_MEMORY)
	// 		{
	// 			for (int j = 0; j < 1024; j++) // page table loop
	// 			{
	// 				ptrPageFN = ptrPage != NULL ? (uint32)*ptrPage : -1;
	// 				// cprintf("%d\n", ptrPageFN);
	// 				if (ptrPageFN == -1)
	// 				{
	// 					cprintf("Enter Enter Enter Enter Enter Enter");
	// 					struct FrameInfo *fi;
	// 					allocate_frame(&fi);
	// 					map_frame(ptr_page_directory, fi, (uint32)ptrPage, PERM_WRITEABLE);
	// 					allocated = 1;
	// 					break;
	// 				}
	// 				ptrPage += 1;
	// 			}
	// 			cprintf("quit third\n");
	// 		}
	// 		else
	// 		{
	// 			cprintf("Create Page Table\n");
	// 			create_page_table(ptr_page_directory, (uint32)va);
	// 		}
	// 		ptrDirFN = va != NULL ? (uint32)*va : -1;
	// 		va += 1;
	// 		if (allocated)
	// 			break;
	// 		// if (ptrDirFN != NULL)
	// 		// {
	// 		// 	uint32 *ptrPage;
	// 		// 	uint32 ptrPageFN; // f# in an entry in the page table
	// 		// 	if (get_page_table(ptr_page_directory, (uint32)va, &ptrPage) == TABLE_IN_MEMORY)
	// 		// 	{
	// 		// 		for (int j = 0; j < 1024; j++)
	// 		// 		{
	// 		// 			ptrPageFN = (uint32)ptrPage;
	// 		// 			ptrPage += 1;
	// 		// 			if (ptrPageFN != NULL)
	// 		// 		}
	// 		// 	}
	// 		// }
	// 	}
	// 	cprintf("quit second\n");
	// }
	// cprintf("quit first\n");

	// change this "return" according to your answer
	// kpanic_into_prompt("kmalloc() is not implemented yet...!!");
	return NULL;
}
void kfree(void *virtual_address)
{
	// TODO: [PROJECT'23.MS2 - #04] [1] KERNEL HEAP - kfree()
	// refer to the project presentation and documentation for details
	//  Write your code here, remove the panic and write your code
	panic("kfree() is not implemented yet...!!");
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	// TODO: [PROJECT'23.MS2 - #05] [1] KERNEL HEAP - kheap_virtual_address()
	// refer to the project presentation and documentation for details
	//  Write your code here, remove the panic and write your code
	panic("kheap_virtual_address() is not implemented yet...!!");

	// EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================

	// change this "return" according to your answer
	return 0;
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	// TODO: [PROJECT'23.MS2 - #06] [1] KERNEL HEAP - kheap_physical_address()
	// refer to the project presentation and documentation for details
	//  Write your code here, remove the panic and write your code
	panic("kheap_physical_address() is not implemented yet...!!");

	// change this "return" according to your answer
	return 0;
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

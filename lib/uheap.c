#include <inc/lib.h>

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

struct UserData
{
	void *va;
	uint32 size;
};
struct UserData userPages[NUM_OF_UHEAP_PAGES];

int FirstTimeFlag = 1;
void InitializeUHeap()
{
	if (FirstTimeFlag)
	{
#if UHP_USE_BUDDY
		initialize_buddy();
		cprintf("BUDDY SYSTEM IS INITIALIZED\n");
#endif
		FirstTimeFlag = 0;
	}
}

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=============================================
// [1] CHANGE THE BREAK LIMIT OF THE USER HEAP:
//=============================================
/*2023*/
void *sbrk(int increment)
{
	return (void *)sys_sbrk(increment);
}

//=================================
// [2] ALLOCATE SPACE IN USER HEAP:
//=================================
void *malloc(uint32 size)
{
	//==============================================================
	// DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	if (size == 0)
		return NULL;
	//==============================================================
	// TODO: [PROJECT'23.MS2 - #09] [2] USER HEAP - malloc() [User Side]
	// Write your code here, remove the panic and write your code
	// panic("malloc() is not implemented yet...!!");

	if (size <= DYN_ALLOC_MAX_BLOCK_SIZE)
	{
		return alloc_block(size, DA_FF);
	}

	int numOfPages = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;

	// uint32 va = USER_HEAP_START;
	for (int i = 0; i < NUM_OF_UHEAP_PAGES; i++)
	{
		uint8 sizeAvailable = 1;
		if (userPages[i].va == 0)
		{
			if (numOfPages > NUM_OF_UHEAP_PAGES - i)
			{
				return NULL;
			}

			for (int j = i; j < numOfPages; j++)
			{
				if (userPages[j].va != 0)
				{
					sizeAvailable = 0;
					break;
				}
			}

			if (sizeAvailable == 0)
			{
				// va += PAGE_SIZE;
				continue;
			}

			sys_allocate_user_mem(KERNEL_HEAP_START + i * PAGE_SIZE, numOfPages * PAGE_SIZE);
			return (void *)(KERNEL_HEAP_START + i * PAGE_SIZE);
		}

		// va += PAGE_SIZE;
		// if (va >= KERNEL_HEAP_MAX)
		// 	break;
	}

	return NULL;
	// Use sys_isUHeapPlacementStrategyFIRSTFIT() and	sys_isUHeapPlacementStrategyBESTFIT()
	// to check the current strategy
}

//=================================
// [3] FREE SPACE FROM USER HEAP:
//=================================
void free(void *virtual_address)
{
	// TODO: [PROJECT'23.MS2 - #11] [2] USER HEAP - free() [User Side]
	//  Write your code here, remove the panic and write your code
	panic("free() is not implemented yet...!!");
}

//=================================
// [4] ALLOCATE SHARED VARIABLE:
//=================================
void *smalloc(char *sharedVarName, uint32 size, uint8 isWritable)
{
	//==============================================================
	// DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	if (size == 0)
		return NULL;
	//==============================================================
	panic("smalloc() is not implemented yet...!!");
	return NULL;
}

//========================================
// [5] SHARE ON ALLOCATED SHARED VARIABLE:
//========================================
void *sget(int32 ownerEnvID, char *sharedVarName)
{
	//==============================================================
	// DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	//==============================================================
	// Write your code here, remove the panic and write your code
	panic("sget() is not implemented yet...!!");
	return NULL;
}

//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//=================================
// REALLOC USER SPACE:
//=================================
//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_move_user_mem(...)
//		which switches to the kernel mode, calls move_user_mem(...)
//		in "kern/mem/chunk_operations.c", then switch back to the user mode here
//	the move_user_mem() function is empty, make sure to implement it.
void *realloc(void *virtual_address, uint32 new_size)
{
	//==============================================================
	// DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	//==============================================================

	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");
	return NULL;
}

//=================================
// FREE SHARED VARIABLE:
//=================================
//	This function frees the shared variable at the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from main memory then switch back to the user again.
//
//	use sys_freeSharedObject(...); which switches to the kernel mode,
//	calls freeSharedObject(...) in "shared_memory_manager.c", then switch back to the user mode here
//	the freeSharedObject() function is empty, make sure to implement it.
void sfree(void *virtual_address)
{
	// Write your code here, remove the panic and write your code
	panic("sfree() is not implemented yet...!!");
}

//==================================================================================//
//========================== MODIFICATION FUNCTIONS ================================//
//==================================================================================//

void expand(uint32 newSize)
{
	panic("Not Implemented");
}

void shrink(uint32 newSize)
{
	panic("Not Implemented");
}

void freeHeap(void *virtual_address)
{
	panic("Not Implemented");
}

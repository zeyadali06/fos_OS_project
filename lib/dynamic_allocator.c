/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//=====================================================
// 1) GET BLOCK SIZE (including size of its meta data):
//=====================================================
uint32 get_block_size(void *va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1);
	return curBlkMetaData->size;
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
int8 is_free_block(void *va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1);
	return curBlkMetaData->is_free;
}

//===========================================
// 3) ALLOCATE BLOCK BASED ON GIVEN STRATEGY:
//===========================================
void *alloc_block(uint32 size, int ALLOC_STRATEGY)
{
	void *va = NULL;
	switch (ALLOC_STRATEGY)
	{
	case DA_FF:
		va = alloc_block_FF(size);
		break;
	case DA_NF:
		va = alloc_block_NF(size);
		break;
	case DA_BF:
		va = alloc_block_BF(size);
		break;
	case DA_WF:
		va = alloc_block_WF(size);
		break;
	default:
		cprintf("Invalid allocation strategy\n");
		break;
	}
	return va;
}

//===========================
// 4) PRINT BLOCKS LIST:
//===========================
void print_blocks_list(struct MemBlock_LIST list)
{
	cprintf("=========================================\n");
	struct BlockMetaData *blk;
	cprintf("\nDynAlloc Blocks List:\n");
	LIST_FOREACH(blk, &list)
	{
		cprintf("(size: %d, isFree: %d) pointer: %x\n", blk->size, blk->is_free, blk + 1);
	}
	cprintf("=========================================\n");
}

//
////********************************************************************************//
////********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

bool is_initialized = 0;
//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{
	//=========================================
	// DON'T CHANGE THESE LINES=================
	if (initSizeOfAllocatedSpace == 0)
		return;
	//=========================================
	//=========================================
	is_initialized = 1;
	// TODO: [PROJECT'23.MS1 - #5] [3] DYNAMIC ALLOCATOR - initialize_dynamic_allocator()

	// IB->size = initSizeOfAllocatedSpace;
	// IB->is_free = 1;

	struct BlockMetaData *FB = (struct BlockMetaData *)daStart;
	FB->size = initSizeOfAllocatedSpace;
	FB->is_free = 1;
	LIST_INIT(&list);
	LIST_INSERT_HEAD(&list, FB);
}

//=========================================
// [4] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *alloc_block_FF(uint32 size)
{
	// TODO: [PROJECT'23.MS1 - #6] [3] DYNAMIC ALLOCATOR - alloc_block_FF()
	// panic("alloc_block_FF is not implemented yet");

	struct BlockMetaData *blkPtr;

	if (size == 0)
	{
		return NULL;
	}

	if (!is_initialized)
	{
		uint32 required_size = size + sizeOfMetaData();
		uint32 da_start = (uint32)sbrk(required_size);
		// get new break since it's page aligned! thus, the size can be more than the required one
		uint32 da_break = (uint32)sbrk(0);
		initialize_dynamic_allocator(da_start, da_break - da_start);
	}

	LIST_FOREACH(blkPtr, &list)
	{
		if (blkPtr->is_free == 1)
		{
			if (blkPtr->size == size + sizeOfMetaData())
			{
				blkPtr->is_free = 0;
				// cprintf("%x %x %x %x\n", (void *)blkPtr, ((void *)blkPtr + sizeOfMetaData()), freeMD, size);
				return (void*)(blkPtr + 1);
			}
			else if (blkPtr->size > size + sizeOfMetaData())
			{

				if ((blkPtr->size - (size + sizeOfMetaData())) < sizeOfMetaData())
				{
					// cprintf("----------------------------------------------------------------------\n");
					blkPtr->is_free = 0;
					return (void*)(blkPtr + 1);
				}
				struct BlockMetaData *freeMD = (struct BlockMetaData *)((void *)blkPtr + size + sizeOfMetaData());
				freeMD->is_free = 1;
				freeMD->size = blkPtr->size - (size + sizeOfMetaData());
				// else
				// {
				// 	// cprintf("----------------------------------------------------------------------");
				// 	blkPtr->size = 0;
				// }
				blkPtr->is_free = 0;
				blkPtr->size = size + sizeOfMetaData();
				LIST_INSERT_AFTER(&list, blkPtr, freeMD);
				// cprintf("%x %x %x %x\n", (void *)blkPtr, (void *)((void *)blkPtr + sizeOfMetaData()), freeMD, size);
				return (void*)(blkPtr + 1);
			}
		}
	}

	struct BlockMetaData *lastMD = (struct BlockMetaData *)(sbrk(size + sizeOfMetaData()));

	if ((void *)lastMD == (void *)-1)
	{
		return NULL;
	}

	lastMD->is_free = 1;
	// lastMD->size = ROUNDUP(size + sizeOfMetaData(), PAGE_SIZE);
	// lastMD->size = size + sizeOfMetaData();
	lastMD->size = (uint32)(sbrk(0) - (void *)lastMD);
	LIST_INSERT_TAIL(&list, lastMD);
	if (lastMD->size == size + sizeOfMetaData())
			{
				lastMD->is_free = 0;
				// cprintf("%x %x %x %x\n", (void *)blkPtr, ((void *)blkPtr + sizeOfMetaData()), freeMD, size);
				return (void*)(lastMD + 1);
			}
	else if (lastMD->size > size + sizeOfMetaData())
			{

				if ((lastMD->size - (size + sizeOfMetaData())) < sizeOfMetaData())
				{
					// cprintf("----------------------------------------------------------------------\n");
					lastMD->is_free = 0;
					return (void*)(lastMD + 1);
				}
				struct BlockMetaData *freeMD = (struct BlockMetaData *)((void *)lastMD + size + sizeOfMetaData());
				freeMD->is_free = 1;
				freeMD->size = lastMD->size - (size + sizeOfMetaData());
				// else
				// {
				// 	// cprintf("----------------------------------------------------------------------");
				// 	blkPtr->size = 0;
				// }
				lastMD->is_free = 0;
				lastMD->size = size + sizeOfMetaData();
				LIST_INSERT_AFTER(&list, lastMD, freeMD);
				// cprintf("%x %x %x %x\n", (void *)blkPtr, (void *)((void *)blkPtr + sizeOfMetaData()), freeMD, size);
				return (void*)(lastMD + 1);
			}
	// free_block((void *)(lastMD + 1));
	return alloc_block_FF(size);
	// return (void *)(lastMD + 1);
}

//=========================================
// [5] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void *alloc_block_BF(uint32 size)
{
	// TODO: [PROJECT'23.MS1 - BONUS] [3] DYNAMIC ALLOCATOR - alloc_block_BF()

	// cprintf("alloc block BF \n");
	struct BlockMetaData *blkPtr;

	if (size == 0)
	{
		return NULL;
	}
	struct BlockMetaData *fit = list.lh_first;
	uint8 isFit = 0;
	LIST_FOREACH(blkPtr, &list)
	{
		if (blkPtr->is_free == 1 && blkPtr->size >= size + sizeOfMetaData())
		{
			isFit = 1;
			fit = blkPtr;

			break;
		}
	}
	// cprintf("begin nextfit size %d \n", nextFit->size);
	// cprintf("begin fit size %d \n", fit->size);
	// cprintf("user size  %d \n", size);
	LIST_FOREACH(blkPtr, &list)
	{

		if (blkPtr->is_free == 1)
		{
			if (blkPtr->size == size + sizeOfMetaData())
			{
				blkPtr->is_free = 0;
				return ((void *)blkPtr + sizeOfMetaData());
			}

			if (blkPtr->size > size + sizeOfMetaData())
			{

				// cprintf("in condition fit 1 \n");
				// if (!isFit)
				// {
				// 	cprintf("nextfit size %d \n", blkPtr->size);
				// 	cprintf("fit size %d \n", fit->size);
				// }
				if (fit->size >= blkPtr->size)
				{
					// cprintf("in condition fit 2 \n");
					fit = blkPtr;
				}
				// cprintf("in condition fit 3 \n");
			}
		}
	}

	// cprintf("after loop \n");
	// cprintf("fit size %d \n", fit->size);
	// cprintf("Test size %d \n", size);
	// cprintf("fit address %x \n", (void *)fit);

	if (isFit == 1)
	{

		struct BlockMetaData *freeMD = (struct BlockMetaData *)((void *)fit + size + sizeOfMetaData());
		freeMD->is_free = 1;
		freeMD->size = fit->size - (size + sizeOfMetaData());
		if ((freeMD->size) < sizeOfMetaData())
		{
			fit->size = freeMD->size;
		}
		else
		{
			fit->size = 0;
			LIST_INSERT_AFTER(&list, fit, freeMD);
		}

		// cprintf("isFit 1 \n");
		fit->is_free = 0;
		fit->size += size + sizeOfMetaData();
		// cprintf("iFit 2 \n");
		// cprintf("%x %x %x %x\n", (void *)fit, ((void *)fit + sizeOfMetaData()), freeMD, size);
		return ((void *)fit + sizeOfMetaData());
	}
	// cprintf("after isFit \n");
	if (sbrk(size + sizeOfMetaData()) != (void *)-1)
	{
		return alloc_block_BF(size);
	}
	else
	{
		// cprintf("size %d /n", );

		return NULL;
	}
}

//=========================================
// [6] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size)
{
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [7] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size)
{
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}

// ===================================================
// [8] FREE BLOCK WITH COALESCING:
// ===================================================
void free_block(void *va)
{
	// cprintf("free_block \n");
	// TODO: [PROJECT'23.MS1 - #7] [3] DYNAMIC ALLOCATOR - free_block()
	if (va == NULL)
		return;

	struct BlockMetaData *myblc = ((struct BlockMetaData *)va - 1);
	if (myblc->is_free == 0)
	{

		myblc->is_free = 1;

		if (LIST_PREV(myblc) != NULL)
		{
			// cprintf("init prev fun \n");

			if (LIST_PREV(myblc)->is_free)
			{

				LIST_PREV(myblc)->size += myblc->size;

				myblc->size = 0;
				myblc->is_free = 0;
				list.size--;
				if (LIST_NEXT(myblc) != NULL)
				{
					LIST_NEXT(LIST_PREV(myblc)) = LIST_NEXT(myblc);
					LIST_PREV(LIST_NEXT(myblc)) = LIST_PREV(myblc);
				}
				// if (LIST_NEXT(myblc) != NULL)
				// 	LIST_NEXT(LIST_PREV(myblc)) = LIST_NEXT(myblc);
				else
					LIST_NEXT(LIST_PREV(myblc)) = NULL;
				myblc = LIST_PREV(myblc);
				// LIST_REMOVE(&list,LIST_PREV(myblc));
			}
			// cprintf("end prev fun \n");
		}
		if (LIST_NEXT(myblc) != NULL)
		{

			// cprintf(" init next fun \n");
			if (LIST_NEXT(myblc)->is_free)
			{

				myblc->size += LIST_NEXT(myblc)->size;
				LIST_NEXT(myblc)->is_free = 0;
				LIST_NEXT(myblc)->size = 0;
				list.size--;

				if (LIST_NEXT(LIST_NEXT(myblc)) != NULL)
				{
					LIST_NEXT(myblc) = LIST_NEXT(LIST_NEXT(myblc));
					LIST_PREV(LIST_NEXT(myblc)) = myblc;
				}
				// if (LIST_NEXT(LIST_NEXT(myblc)) != NULL)
				// 	LIST_NEXT(myblc) = LIST_NEXT(LIST_NEXT(myblc));
				else
					LIST_NEXT(myblc) = NULL;

				// LIST_REMOVE(&list, LIST_NEXT(myblc));
			}
			// cprintf(" end next fun \n");
		}
	}

	// panic("free_block is not implemented yet");
}

//=========================================
// [4] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void *va, uint32 new_size)
{
	// TODO: [PROJECT'23.MS1 - #8] [3] DYNAMIC ALLOCATOR - realloc_block_FF()
	// //panic("realloc_block_FF is not implemented yet");

	// print_blocks_list(list);

	if (va == NULL)
	{
		return alloc_block(new_size, DA_FF);
	}
	if (va != NULL && new_size == 0)
	{
		free_block(va);
		return NULL;
	}

	struct BlockMetaData *currentBlk = ((struct BlockMetaData *)va - 1);

	// decrease size
	if (new_size + sizeOfMetaData() < get_block_size(va))
	{
		// struct BlockMetaData *nextBlk = LIST_NEXT(currentBlk);
		// struct BlockMetaData *prevBlk = LIST_PREV(currentBlk);
		struct BlockMetaData *freeMD = (struct BlockMetaData *)((void *)currentBlk + new_size + sizeOfMetaData());
		freeMD->is_free = 0;
		freeMD->size = currentBlk->size - (new_size + sizeOfMetaData());
		LIST_INSERT_AFTER(&list, currentBlk, freeMD);
		currentBlk->size = new_size + sizeOfMetaData();
		free_block((struct BlockMetaData *)freeMD + 1);
		return va;
	}
	// increase size
	else if (new_size + sizeOfMetaData() > get_block_size(va))
	{
		// cprintf("1\n");
		if (LIST_NEXT(currentBlk) != NULL)
		{
			if (LIST_NEXT(currentBlk)->is_free && LIST_NEXT(currentBlk)->size + currentBlk->size - sizeOfMetaData() >= new_size + sizeOfMetaData())
			{

				// cprintf("2\n");
				int currSize = currentBlk->size;
				int nextSize = (LIST_NEXT(currentBlk)->size + currSize) - (new_size + sizeOfMetaData());
				currentBlk->size = new_size + sizeOfMetaData();
				LIST_NEXT(currentBlk)->size = 0;
				LIST_NEXT(currentBlk)->is_free = 0;
				LIST_NEXT(currentBlk) = (struct BlockMetaData *)((void *)currentBlk + currentBlk->size);
				LIST_NEXT(currentBlk)->is_free = 1;
				LIST_NEXT(currentBlk)->size = (currSize + nextSize) - new_size + sizeOfMetaData();

				return va;

				// int currSize = currentBlk->size;
				// int nextSize = (LIST_NEXT(currentBlk)->size + currSize) - (new_size + sizeOfMetaData());
				// currentBlk->size = new_size + sizeOfMetaData();
				// LIST_NEXT(currentBlk)->size = 0;
				// LIST_NEXT(currentBlk)->is_free = 0;
				// if (nextSize < sizeOfMetaData())
				// {
				// 	cprintf("ss");
				// 	if (LIST_NEXT(LIST_NEXT(currentBlk)) != NULL)
				// 	{
				// 		LIST_NEXT(currentBlk) = LIST_NEXT(LIST_NEXT(currentBlk));
				// 		LIST_PREV(LIST_NEXT(currentBlk)) = currentBlk;
				// 	}
				// 	else
				// 	{
				// 		LIST_NEXT(currentBlk) = NULL;
				// 	}
				// 	currentBlk->size += nextSize;
				// }
				// else
				// {
				// 	cprintf("mm");
				// 	LIST_NEXT(currentBlk) = (struct BlockMetaData *)((void *)currentBlk + currentBlk->size);
				// 	LIST_NEXT(currentBlk)->is_free = 1;
				// 	LIST_NEXT(currentBlk)->size = (currSize + nextSize) - new_size + sizeOfMetaData();
				// }
				// return va;
			}
			if (LIST_NEXT(currentBlk)->is_free && LIST_NEXT(currentBlk)->size + currentBlk->size >= new_size + sizeOfMetaData())
			{
				// cprintf("3\n");
				currentBlk->size = currentBlk->size + LIST_NEXT(currentBlk)->size;
				LIST_NEXT(currentBlk)->size = 0;
				LIST_NEXT(currentBlk)->is_free = 0;
				if (LIST_NEXT(LIST_NEXT(currentBlk)) != NULL)
				{
					LIST_NEXT(currentBlk) = LIST_NEXT(LIST_NEXT(currentBlk));
					LIST_PREV(LIST_NEXT(currentBlk)) = currentBlk;
				}
				else
				{
					LIST_NEXT(currentBlk) = NULL;
				}
				currentBlk->is_free = 0;
				list.size--;
				// cprintf("%x %x\n",LIST_PREV(LIST_NEXT(currentBlk)) ,currentBlk);
				// LIST_REMOVE(&list, LIST_NEXT(currentBlk));
				return va;
			}
		}
		// cprintf("4\n");
		free_block((void *)((struct BlockMetaData *)currentBlk + 1));
		// LIST_REMOVE(&list, currentBlk);

		// 	currentBlk->size = 0;
		// 	currentBlk->is_free = 0;
		// 	list.size--;
		// 	if (LIST_PREV(currentBlk) == NULL)
		// 	{
		// 		if (LIST_SIZE(&list) > 1)
		// 		{
		// 			LIST_INSERT_HEAD(&list, LIST_NEXT(currentBlk));
		// 		}
		// 		else
		// 		{
		// 			LIST_INIT(&list);
		// 		}
		// 	}
		// 	else if (LIST_PREV(currentBlk) != NULL && LIST_NEXT(currentBlk) != NULL)
		// 	{
		// 		LIST_NEXT(LIST_PREV(currentBlk)) = LIST_NEXT(currentBlk);
		// 	}
		// 	else if (LIST_NEXT(currentBlk) == NULL)
		// 	{
		// 		LIST_INSERT_TAIL(&list, LIST_PREV(currentBlk));
		// 	}
		// 	LIST_NEXT(currentBlk) = LIST_PREV(currentBlk) = NULL;

		return alloc_block_FF(new_size);
	}
	// cprintf("5\n");
	return va;
}

#include <inc/dynamic_allocator.h>

#ifndef FOS_INC_UHEAP_H
#define FOS_INC_UHEAP_H 1

// Values for user heap placement strategy
#define UHP_PLACE_FIRSTFIT 0x1
#define UHP_PLACE_BESTFIT 0x2
#define UHP_PLACE_NEXTFIT 0x3
#define UHP_PLACE_WORSTFIT 0x4

// 2020
#define UHP_USE_BUDDY 0

void *malloc(uint32 size);
void *smalloc(char *sharedVarName, uint32 size, uint8 isWritable);
void *sget(int32 ownerEnvID, char *sharedVarName);
void free(void *virtual_address);
void sfree(void *virtual_address);
void *realloc(void *virtual_address, uint32 new_size);
void print_user_data();

#define arrSize ((((NUM_OF_UHEAP_PAGES * PAGE_SIZE) - DYN_ALLOC_MAX_SIZE) - PAGE_SIZE) / PAGE_SIZE)

#define MAX_FREE_SPACE()                  \
    ({                                    \
        int num = 0;                      \
        int big = 0;                      \
        for (int i = 0; i < arrSize; i++) \
        {                                 \
            if (userPages[i].size == 0)   \
            {                             \
                num++;                    \
            }                             \
            else                          \
            {                             \
                num = 0;                  \
            }                             \
            if (big < num)                \
            {                             \
                big = num;                \
            }                             \
        }                                 \
        (big * PAGE_SIZE);                \
    })

struct UserData
{
    void *va;
    uint32 size;
};
struct UserData userPages[arrSize];

#endif

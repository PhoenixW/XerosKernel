/* mem.c : memory manager
 */

#include <xeroskernel.h>
#include "i386.h"

/* Your code goes here */

extern  long	freemem; 	/* start of free memory (set in i386.c) */
extern  char	*maxaddr;	/* max memory address (set in i386.c)	*/

/*The struct that will make up the linked list of free memory.
* The size of the struct is 16 bytes.
*/
struct memHeader{
    unsigned long size;
    struct memHeader* prev;
    struct memHeader* next;
    char* sanityCheck;
    unsigned char dataStart[0];
};

static struct memHeader* freeList;
static char* const sanityCheck = (char*)0xAE84C92D;
static const size_t MEM_HEADER_SIZE = sizeof(struct memHeader);

/*
* Returns a 16-byte aligned address.  If the passed address is already aligned, it
* returns the passed address.  Otherwise, it will return the smallest address larger
* than the passed address which is 16-byte aligned.
*/
static void* alignAddress(void* addr){
    return (void*)((size_t)addr + (16 - (size_t)addr % 16) %16) ;
}

/*
* Check that a passed address is valid
*/
Bool isValidUserAddress(void* addr){
    // Check if address is in the OS space
    if((char*)addr < (char*)freemem){
        // kprintf("1\n");
        return FALSE;
    }
    // Check if address exceeds available memory
    if((char*)addr > (char*)maxaddr){
        // kprintf("2\n");
        return FALSE;
    }
    // Check if address is in the hole
    if((size_t)addr > HOLESTART && (size_t)addr < HOLEEND){
        // kprintf("3\n");
        return FALSE;
    }
    return TRUE;
}

/*
* Check that a passed address is valid
*/
Bool isValidKernelAddress(void* addr){
    // Check if address is in the OS space
    if((char*)addr > (char*)freemem){
        // kprintf("1\n");
        return FALSE;
    }
    return TRUE;
}

/*
* This function initializes the state of the memory manager.
*/
void kmeminit(void){
    // At first, there should be two blocks: from freemem to HOLESTART and from HOLEEND to end
    freeList = (struct memHeader*) alignAddress((void*)freemem);
    // IMPORTANT: the size should be the allocated size (without the header size)
    freeList->size = HOLESTART - (size_t)freeList - sizeof(struct memHeader);
    freeList->prev = NULL;
    freeList->sanityCheck = sanityCheck;
    freeList->next = (struct memHeader*) alignAddress((void*)HOLEEND);

    //maxaddr is not aligned so lets align it right away
    maxaddr = (char*)((unsigned long)maxaddr - ((unsigned long) maxaddr % 16));
    freeList->next->size = (unsigned long)maxaddr - (unsigned long)freeList->next - sizeof(struct memHeader);
    freeList->next->prev = freeList;
    freeList->next->next = NULL;
    freeList->next->sanityCheck = sanityCheck;
}

/*
* This function allocates a block of memory to the calling process.  The size
* of the memory block allocated is passed as the only parameter.  The address
* of the start of the memory block is returned. 0 is returned if there is not enough
* free space to satisfy the request.
*/
void* kmalloc(size_t size){

    // Make sure that size is positive
    if(size <= 0){
        return 0;
    }

    // Determine the size of space to allocate (should be 16-byte aligned)
    unsigned long reqSize = (unsigned long)size + ((16 - size%16) %16);

    // Iterate over the free list
    for(struct memHeader* memBlock = freeList; memBlock != NULL; memBlock = memBlock->next){

        // Check if there is enough space at this point in the free list
        if(memBlock->size >= reqSize){

            // Check if there's enough space for a new header; if not, don't create one
            // If remaining free space is too small to be used, fill up the remaining space with original request.
            if(memBlock->size - reqSize < 2 * MEM_HEADER_SIZE){

                reqSize += memBlock->size - reqSize;
                struct memHeader* temp = memBlock->next;

                // Update the freeList
                if(memBlock->next != NULL){
                    memBlock->next->prev = memBlock->prev;
                }

                if(memBlock->prev != NULL){
                    memBlock->prev->next = temp;
                } else {
                    //if memBlock->prev is NULL, temp is the first node in freeList
                    freeList = temp;
                }
            }
            else{
                //Create the new memHeader
                struct memHeader* newFreeBlock = (struct memHeader*) alignAddress((void*)(memBlock->dataStart + reqSize));
                newFreeBlock->size = memBlock->size - reqSize - MEM_HEADER_SIZE;
                newFreeBlock->prev = memBlock->prev;
                newFreeBlock->next = memBlock->next;
                newFreeBlock->sanityCheck = sanityCheck;

                //Update the memHeader's new location in freeList
                if(memBlock->next != NULL){
                    memBlock->next->prev = newFreeBlock;
                }
                if(memBlock->prev != NULL){
                    memBlock->prev->next = newFreeBlock;
                } else {
                    //newFreeBlock is the first memHeader is FreeList
                    freeList = newFreeBlock;
                }
            }

            // Specify the size of the allocated memory block
            memBlock->size = reqSize;

            return memBlock->dataStart;
        }
    }
    return 0;
}

/*
* Check if the address passed to the function was likely allocated by kmalloc.
* Return TRUE if it was likely allocated; FALSE Otherwise.
*/
Bool checkIfAllocated(unsigned char* addr){

    // Check if address is in the OS space
    if(addr < (unsigned char*)freemem){
        // kprintf("1\n");
        return FALSE;
    }
    // Check if address exceeds available memory
    if(addr > (unsigned char*)maxaddr){
        // kprintf("2\n");
        return FALSE;
    }
    // Check if address is in the hole
    if((size_t)addr > HOLESTART && (size_t)addr < HOLEEND){
        // kprintf("3\n");
        return FALSE;
    }
    // Check if address is 16-byte aligned
    if((size_t)addr % 16 != 0 ){
        // kprintf("4\n");
        return FALSE;
    }
    // Check if the size of the allocated address is 16-byte aligned
    if(*(addr - 16) % 16 != 0){
        // kprintf("5\n");
        return FALSE;
    }
    // Check the sanityCheck bytes
    //Changed this to creating an object because just checking the sanityCheck on its own didnt seem to work
    struct memHeader* memHead = (struct memHeader*) (addr - 16);
    if(memHead->sanityCheck != sanityCheck){
        // kprintf("6\n");
        return FALSE;
    }
    return TRUE;
}

/*
* This function returns the chunk of previously allocated memory beginning at ptr,
* and returns it to the free memory pool.  The size is specified in the memory header.
* Returns 1 on success, 0 on failure.
*/
int kfree(void* ptr){
    //kprintf("in kfree; ptr is: %d\n", ptr);
    unsigned char* addr = ptr;
    if(!checkIfAllocated(ptr)){
        // kprintf("failed allocation test\n");
        return 0;
    }

    struct memHeader* freedHeader = (struct memHeader*)(addr - MEM_HEADER_SIZE);
    struct memHeader* nextHeader = (struct memHeader*)(addr + freedHeader->size);
    if((unsigned long)nextHeader == HOLESTART || nextHeader == (struct memHeader*)maxaddr){
        nextHeader = NULL;
    }
    // If there is no free list, start one
    if(freeList == NULL){
        freeList = freedHeader;
        freedHeader->prev = NULL;
        freedHeader->next = NULL;
        return 1;
    }
    // Return address to the free list in the proper place; coallesce adjacent blocks
    for(struct memHeader* memBlock = freeList; memBlock != NULL; memBlock = memBlock->next){
        if(memBlock > freedHeader){
            // Check if can merge the blocks
            if(memBlock == nextHeader){
                freedHeader->prev = nextHeader->prev;
                freedHeader->next = nextHeader->next;
                freedHeader->size += nextHeader->size + MEM_HEADER_SIZE;
                if(nextHeader->prev != NULL){
                    nextHeader->prev->next = freedHeader;
                }else{
                    freeList = freedHeader;
                }
                if(nextHeader->next != NULL){
                    nextHeader->next->prev = freedHeader;
                }
            }
            else{
                freedHeader->prev = memBlock->prev;
                freedHeader->next = memBlock;
                memBlock->prev = freedHeader;

                if(freedHeader->prev != NULL){
                    freedHeader->prev->next = freedHeader;
                }else{
                    freeList = freedHeader;
                }
            }
            // return because you have put it back in the free list.
            return 1;
        // Check if this is the last block in the free list
        }else if(memBlock->next == NULL){
            memBlock->next = freedHeader;
            freedHeader->prev = memBlock;
            // return here so that you don't change the pointers in the next iteration,
            // where memBlock with be freeBlock.
            return 1;
        // Check if can merge with space before freed block
        }else if((size_t)memBlock->dataStart + memBlock->size == (size_t)freedHeader){
            memBlock->size += freedHeader->size + MEM_HEADER_SIZE;
            if(memBlock->next == nextHeader){
                memBlock->size += nextHeader->size + MEM_HEADER_SIZE;
                memBlock->next = nextHeader->next;
                if(memBlock->next != NULL){
                    memBlock->next->prev = memBlock;
                }
            }
            // Need to return so that you don't go past the freedHeader, because if you do, it will
            // change the list, even though you've already changed it here.
            return 1;
        }
    }
    return 1;
}

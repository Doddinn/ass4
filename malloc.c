#define USE_REAL_SBRK 1
#pragma GCC diagnostic ignored "-Wunused-function"

#if USE_REAL_SBRK
#define _GNU_SOURCE

#include <sys/mman.h>
#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#include "malloc.h"

/* Function to allocate heap. Do not modify.
 * This is a wrapper around the system call sbrk.
 * For initialization, you can call this function as allocHeap(NULL, size)
 *   -> It will allocate a heap of size <size> bytes and return a pointer to the start address
 * For enlarging the heap, you can later call allocHeap(heapaddress, newsize)
 *   -> heapaddress must be the address previously returned by allocHeap(NULL, size)
 *   -> newsize is the new size
 *   -> function will return NULL (no more memory available) or heapaddress (if ok)
 */

uint8_t *allocHeap(uint8_t *currentHeap, uint64_t size)
{
        static uint64_t heapSize = 0;
        if( currentHeap == NULL ) {
                uint8_t *newHeap  = sbrk(size);
                if(newHeap)
                        heapSize = size;
                return newHeap;
        }
        uint8_t *newstart = sbrk(size - heapSize);
        if(newstart == NULL) return NULL;
        heapSize += size;
        return currentHeap;
}
#else
#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>

#include "malloc.h"
// This is a "fake" version that you can use on MacOS
// sbrk as used above is not available on MacOS
// and normal malloc allocation does not easily allow resizing the allocated memory
uint8_t *allocHeap(uint8_t *currentHeap, uint64_t size)
{
        static uint64_t heapSize = 0;
        if( currentHeap == NULL ) {
                uint64_t newHeap  = malloc(10*size);
                if(newHeap)
                        heapSize = 10*size;
                return newHeap;
        }
        if(size <= heapSize) return currentHeap;
        return NULL;
}
#endif


/*
 * This is the heap you should use.
 * (Initialized with a heap of size HEAP_SIZE) in initAllocator())
 */
uint8_t *_heapStart = NULL;
uint64_t _heapSize = 0;

/*
 * This should point to the first free block in memory.
 */
Block *_firstFreeBlock;

/*
 * Initializes the memory block. You don't need to change this.
 */
void initAllocator()
{
        _heapStart = allocHeap(NULL, HEAP_SIZE);
        _heapSize = HEAP_SIZE;
        /* Add more initialization below, e.g. for the free block list */
        // See lab tutorial
        if (_heapStart == NULL){
                return;
        };

        _firstFreeBlock = (Block*)_heapStart;
        _firstFreeBlock->size = _heapSize;
        _firstFreeBlock->next = NULL;

}


/*
 * Gets the next block that should start after the current one.
 */
static Block *_getNextBlockBySize(const Block *current)
{
        (void)current;
        return NULL;
        // See lab tutorial
}

/*
 * Dumps the allocator. You should not need to modify this.
 */
void dumpAllocator()
{
        // See lab tutorial
}

/*
 * Round the integer up to the block header size (16 Bytes).
 */
uint64_t roundUp(uint64_t n)
{
        // See lab tutorial
        return (n + 15) & ~15;// n = 6, 6 + 15 = 21, 21= 00010101, ~15 means inverting 15 so  from 00001111 to 11110000.
                              // 21 = 00010101 & 11110000 = 00010000 = 16
}

/* Helper function that allocates a block
 * takes as first argument the address of the next pointer that needs to be updated (!)
 */
static void *allocate_block(Block **update_next, Block *block, uint64_t new_size)
{
        (void)update_next;
        (void)block;
        (void)new_size;
        /* Not mandatory but possibly useful to implement this as a separate function
         * called by my_malloc */
        return NULL;
}

void *my_malloc(uint64_t size)
{
        (void)size;
        /* TODO: Implement */
        size = roundUp(size + sizeof(Block));
        Block *bestBlock = NULL;
        Block **prevLink = &_firstFreeBlock;


                for (Block **current = &_firstFreeBlock; *current != NULL; current = &((*current)->next)){
                        Block *block = *current;
                        if (block->size >= size){
                                if(bestBlock == NULL || block->size < bestBlock->size){
                                        bestBlock = block;
                                        prevLink = current;
                        }
                }
        }

        // if no block has been found then request more memory
        if (bestBlock == NULL){
        // increase size of heap
                uint64_t newsize = _heapSize + HEAP_SIZE;
                _heapStart = allocHeap(_heapStart, newsize);

                //if it fails allocate more memory
                if (_heapStart == NULL){
                        return NULL;
                }
                // update the total sum of the heap size
                _heapSize = newsize;

                // create new free block at the newly allocated memory
                Block *newBlock = (Block *)(_heapStart + _heapSize - HEAP_SIZE);
                //set size of of the new block and insert of the head of the free list
                newBlock->size = HEAP_SIZE;
                newBlock->next = _firstFreeBlock;
                _firstFreeBlock = newBlock;

                return my_malloc(size - sizeof(Block));

        }

        *prevLink = bestBlock->next;

        //if the block is larger and then the req size plus the header and plus the alignment space
        // split the block in two one for allocated mem and other for open free space
        if(bestBlock->size >= size + sizeof(Block) + 16){
                Block *newBlock = (Block *)((uint8_t *)bestBlock + size);
                newBlock->size = bestBlock->size - size;
                newBlock->next = _firstFreeBlock;
                _firstFreeBlock =newBlock;
                bestBlock->size = size;
        }

        return (void*)(bestBlock + 1);

        return NULL;
}


/* Helper function to merge two freelist blocks.
 * Assume: block1 is at a lower address than block2
 * Does nothing if blocks are not neighbors (i.e. if block1 address + block1 size is not block2 address)
 * Otherwise, merges block by merging block2 into block1 (updates block1's size and next pointer
 */
static void merge_blocks(Block *block1, Block *block2)
{
        (void)block1;
        (void)block2;
        /* TODO: Implement */
        /* Note: Again this is not mandatory but possibly useful to put this in a separate
         * function called by my_free */
}


void my_free(void *address)
{
        (void)address;
        /* TODO: implement */

        if (address == NULL){
                return;
        }
        Block *block = (Block *)address - 1;

        // search for where to insert in sorted order
        Block **prevLink = &_firstFreeBlock;
        while(*prevLink != NULL && *prevLink < block){
                prevLink = &((*prevLink)->next);
        }

        //insert block intoo free list
        block->next = *prevLink;
        *prevLink = block;

        //Looking to combine to the next block available
        if (block->next != NULL && (uint64_t *)block + block->size == (uint64_t *)block->next){
                block->size += block->next->size;
                block->next = block->next->next;
        }

        // test to try to merge to the prev block availabel
        if (prevLink != &_firstFreeBlock){
                Block *prev = (Block *)((uint64_t *)prevLink - sizeof(Block));
                if ((uint64_t *)prev + prev->size == (uint64_t *)block){
                        prev->size += block->size;
                        prev->next = block->next;
                }

        }

}
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int has_initialized = 0;

// our memory area we can allocate from, here 64 kB
#define MEM_SIZE (64*1024)
uint8_t heap[MEM_SIZE];

// start and end of our own heap memory area
void *managed_memory_start; 

// this block is stored at the start of each free and used block
struct mem_control_block {
  int size;
  struct mem_control_block *next;  // Points to control block at start of next free area.
};

// pointer to start of our free list
struct mem_control_block *free_list_start;      

void mymalloc_init() { 

	// our memory starts at the start of the heap array
	managed_memory_start = &heap;

	// allocate and initialize our memory control block 
	// for the first (and at the moment only) free block
	struct mem_control_block *m = (struct mem_control_block *)managed_memory_start;
	m->size = MEM_SIZE - sizeof(struct mem_control_block);

	// no next free block
	m->next = (struct mem_control_block *)0;

	// initialize the start of the free list
	free_list_start = m;

	// We're initialized and ready to go
	has_initialized = 1;
}

// Was not sure how to test if a block is null, so this can be changed if syntax is incorrect. 
int block_is_null(struct mem_control_block * block){
	if (block == (struct mem_control_block *)0 || 
		block == (void*)0 || 
		block == NULL){
		return 1;
	}
	return 0;
}

// Return true if given block is free. False otherwise. 
int block_is_free(struct mem_control_block * block){
	
	if (block_is_null(block)){
		// Given block can't be null
		return 0;
	}

	// Iterate through all free blocks and look for a match 
	struct mem_control_block *m = free_list_start;
	while(!block_is_null(m)){
		if (m == block){
			return 1;
		}
		m = m->next;
	}
	return 0;
}

// Return next neighbour of given block if it exists. Returns null if it does not exist. 
// A neighbour can be free or in use. 
struct mem_control_block* block_next_neighbour(struct mem_control_block * block){
	
	if (block_is_null(block)){
		// Given block can't be null
		return NULL;
	}

	void *block_end_address = ((void *)block) + sizeof(block) + block->size;
	void *memory_end_address = ((void *)managed_memory_start) + MEM_SIZE;

	if (block_end_address > managed_memory_start){
		// This is the last block, so no next neighbours. 
		return NULL;
	}

	// Next block should be directly next in memory, which is the end address of this block
	return (struct mem_control_block*) block_end_address;

}

// Return previous neighbour of given block if it exists. Returns null if it does not exist. 
// A neighbour can be free or in use. 
struct mem_control_block* block_previous_neighbour(struct mem_control_block * block){
	if (block_is_null(block)){
		// Given block can't be null
		return NULL;
	}

	if ((void *)block < managed_memory_start){
		// This is the first block, so no previous neighbour
		return NULL;
	}
	
	// Iterate through all blocks and look for given block 
	struct mem_control_block *current_block = managed_memory_start;
	while(!block_is_null(current_block)){
		struct mem_control_block* next_block = block_next_neighbour(current_block);

		if (next_block == block){
			// next_block is the given block, so current_block is previous neighbour to given block
			return current_block;
		}

		// Match not found yet, continue with next block
		current_block = next_block;
	}

	// Something wrong happened if we reached this. 
	printf("ERROR: Unable to find previous neighbour of block %p", block);
	return NULL;
}

// Return previous FREE block of given block if it exists. Returns null if it does not exist. 
// This does not have to be a neighbour. 
struct mem_control_block* block_previous_free_block(struct mem_control_block * block){
	if (block_is_null(block)){
		// Given block can't be null
		return NULL;
	}
	
	// Iterate through all previous blocks of given block, until a free one is found
	struct mem_control_block* current_block = block_previous_neighbour(block);

	do {
		if (block_is_free(current_block)){
			// The current block is free, return it!
			return current_block;
		}
		// The current block was not free, check the previous
		current_block = block_previous_neighbour(current_block);
	} while(current_block != NULL);

	// No previous free blocks have been found, return null
	return NULL;
}

// Return next FREE block of given block if it exists. Returns null if it does not exist. 
// This does not have to be a neighbour. 
struct mem_control_block* block_next_free_block(struct mem_control_block * block){
	
	if (block_is_null(block)){
		// Given block can't be null
		return NULL;
	}

	struct mem_control_block* next_free_block = block->next;

	if (block_is_null(next_free_block)){
		// Return null instead of null pointer
		return NULL;
	}
	return next_free_block;

	/* 
	TOOD: This can be removed in final version. 

	This is a manual way of doing it, but not necessary as
	block->next should return the same, if blocks are correct. 
	Will leave it here for test purposes. 
	
	// Iterate through all next blocks of given block, until a free one is found
	struct mem_control_block* current_block = block_next_neighbour(block);

	do {
		if (block_is_free(current_block)){
			// The current block is free, return it!
			return current_block;
		}
		// The current block was not free, check the next
		current_block = block_next_neighbour(current_block);
	} while(current_block != NULL);

	// No next free blocks have been found, return null
	return NULL;
	*/
}



void *mymalloc(long numbytes) {
	if (has_initialized == 0) {
		mymalloc_init();
	}
	// make sure numbytes is devisible by 8, for correct memory allocation.
	while(numbytes%8 != 0){
		numbytes++;
	}

	// Calculate the size required by data and metadata
	long total_block_size = numbytes + sizeof(struct mem_control_block);

	// Iterate through all free blocks, and choose the first with enough space
	struct mem_control_block* chosen_block;
	struct mem_control_block* current_block = free_list_start;
	while (!block_is_null(current_block)){
		if (current_block->size >= total_block_size){
			chosen_block = current_block;
			break;
		}
		current_block = block_next_free_block(current_block);
	}

	// Check that we found a block
	if (block_is_null(chosen_block)){
		printf("ERROR: Unable to find a suitable block for allocating");
		return (void *)0;
	}

	// A suitable block has been chosen. 
	// Split the chosen block into two new blocks, one occupied and one free. 
	
	struct mem_control_block* occupied_block = chosen_block;	// Same start as chosen block
	struct mem_control_block* free_block = ((void*)occupied_block) + total_block_size;	// Starts where occupied block ends

	// Update size and next attributes
	occupied_block->size = numbytes;
	occupied_block->next = free_block;
	
	free_block->size = chosen_block->size - total_block_size - sizeof(struct mem_control_block);
	free_block->next = chosen_block->next;

	// In case chosen block was the first element in free list,
	// Free list needs to be updated to point to the new free block. (Not the occupied one)
	if (chosen_block == free_list_start){
		free_list_start = free_block;
	}

	// Make sure the previous block also points to the free block
	struct mem_control_block* free_block_previous_neighbour = block_previous_neighbour(free_block);
	if (free_block_previous_neighbour != NULL){  // In case prev doesn't exist.
		free_block_previous_neighbour->next = free_block;
	}

	// Return pointer to allocated memory.
	return occupied_block;
}

// Helper function for the free function.
struct mem_control_block * get_next(struct mem_control_block * block){
	// Return next control block. 
	// TODO: return null if no next block exists
	return (struct mem_control_block *)((void *)block + sizeof(block) + block->size);
}

void myfree(void *firstbyte) {

  	// First, we need to look for adjacent free blocks, in case we need to combine.
  	// Create pointer to first free control block. 
  
  	struct mem_control_block *block_to_free = (struct mem_control_block *)firstbyte;
  	// Potentially free neighbours.
  	struct mem_control_block *next_control_block = (void *)0;
  	struct mem_control_block *previous_control_block = (void *)0;


  	// Determine if the neighours are free. If not they are left as zero.
  	struct mem_control_block *m = free_list_start;
	while(m != (void *)0){
		if (get_next(m) == block_to_free){
			previous_control_block = m;
		}
		else if (get_next(block_to_free) == m){
			next_control_block = m;
		}
		m = m->next;
	}

	int prev_free = previous_control_block != (void *)0;
    int next_free = next_control_block != (void *)0;
    printf("prev_free: %d\n", prev_free);
    printf("next_free: %d\n", next_free);

	// CASE 1: Both neighbours are free
	if (prev_free && next_free){
		printf("This is case 1\n");
		previous_control_block->next = next_control_block->next;
		// TODO: Find number of blocks, x*sizeof(struct)
		previous_control_block->size = previous_control_block->size + block_to_free->size + next_control_block->size + 2 * sizeof(struct mem_control_block *);
	}
  
    // CASE 2: Previous free, next occupied
    else if (prev_free && !next_free){
		printf("This is case 2\n");

        previous_control_block->size = previous_control_block->size + block_to_free->size + sizeof(struct mem_control_block *);
    }

	// CASE 3: Previous occupied, next free
	else if (!prev_free && next_free){
		printf("This is case 3\n");

		// Make the previous free block point to the block being freed. 
		struct mem_control_block *m = free_list_start;
		while(m != (void *)0){
			if (m == next_control_block){
				m->next = block_to_free;
				break;
			}
			m = m->next;
    	}   
		// Update size of the free area
		block_to_free->size = block_to_free->size + next_control_block->size + sizeof(struct mem_control_block *);

		// In case this becomes the first free block.
		if (free_list_start > block_to_free){
			free_list_start = block_to_free;
		}
  	}

    // CASE 4: No neighbours are free
  	else if (!prev_free && !next_free){
		printf("This is case 4\n");
		// Make the previous free block point to the block being freed. 
		struct mem_control_block *m = free_list_start;
		while(m != (void *)0){
			if (m == next_control_block){
				m->next = block_to_free;
				break;
			}
			m = m->next;
		}
		// The size is good.

		// In case this becomes the first free block.
		if (free_list_start > block_to_free){
			free_list_start = block_to_free;
		}
  	}
  	else {
    	printf("Failed to free");
  	}

}


void printfree(){
	printf("-------------------\nPrinting free blocks:\n");
	int counter = 1;
    struct mem_control_block *m = free_list_start;
	while(m != (void *)0){
		printf("%d: %d (%p)\n", counter, m->size, m->next);
		m = m->next;
		counter++;
	}
	printf("-------------------\n");
}	



void testCase_1(){
  /* TEST CASE 1
    Add a few bytes first, then try to insert a too large element.
    We expect the few bytes to be success, then the last one to fail - as there is no room.
  */
  void *p;
  p = mymalloc(42); // allocate 42 bytes
  if (p != (void *)0){
    printf("SUCCESS!\n");
  } else{
    printf("FAILED!\n");
  }

  
  p = mymalloc(48); // allocate 42 bytes
  if (p != (void *)0){
    printf("SUCCESS!\n");
  } else{
    printf("FAILED!\n");
  }

  
  p = mymalloc(64*1024); // allocate 42 bytes
  if (p != (void *)0){
    printf("SUCCESS!\n");
  } else{
    printf("FAILED!\n");
  }
}

void testCase_2(){
	// start by filling our area with some data. No edge cases.
    mymalloc(42); // allocate 42 bytes
	void *p2;
	p2 = mymalloc(102); // allocate 42 bytes
	void *p3;
	p3 = mymalloc(17); // allocate 42 bytes
	void *p4;
	p4 = mymalloc(90); // allocate 42 bytes
	void *p5;
	p5 = mymalloc(24); // allocate 42 bytes

	// Should only be one last big free block at the end.
	printfree();
	
	//Testing case 4: no free neighbours
	myfree(p2);
	printfree();


	//Testing case 2: Previous free, next occupied
	myfree(p3);
    printfree();

    //Testing case 3: Previous occupied, next free(rest of the heap)
    myfree(p5);
    printfree();
    
	//Testing case 1: Previous free, next free
    myfree(p4);
    printfree();

}


int main(int argc, char **argv) {
    /* add your test cases here! */
  
    /* TEST CASE 1
		Add a few bytes first, then try to insert a too large element.
		We expect the few bytes to be success, then the last one to fail - as there is no room.
    */

    //testCase_1();
    
    /* TEST CASE 2
		Test all 4 cases in the free funciton.
    */
    testCase_2();

    return 0;
}


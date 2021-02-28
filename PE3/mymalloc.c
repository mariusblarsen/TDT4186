#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int has_initialized = 0;

/* NOTES

	-	For some reason sizeof(block_instance) != sizeof(struct mem_control_block) 
		where block_instance is an instance of struct mem_control_block.  
		Not quite sure why, but using the latter works. Maybe if next pointer is null, then it is not counted? 
*/

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

	void *block_end_address = ((void *)block) + sizeof(struct mem_control_block) + block->size;
	void *memory_end_address = ((void *)managed_memory_start) + MEM_SIZE;

	if (block_end_address >= memory_end_address){
		// This is the last block, so no next neighbours. 
		return NULL;
	}

	// Next block should be directly next in memory, which is the end address of this block
	return (struct mem_control_block*) block_end_address;
}

// Print all blocks 
void block_print_all(){

	if (has_initialized == 0) {
		mymalloc_init();
	}
	// printf("\nNUMBER\tADDRESS\t\tSIZE\tNEXT\t\tTYPE\n");
	printf("\n");
	printf("|-------+-----------------------+-------+-----------------------+---------------|\n");
	printf("| ID\t");
	printf("| START\t\t\t");
	printf("| SIZE\t");
	printf("| NEXT\t\t\t");
	printf("| TYPE\t\t|");
	printf("\n");
	printf("|-------+-----------------------+-------+-----------------------+---------------|\n");

	struct mem_control_block* current_block = managed_memory_start;
	int counter = 0;
	while(!block_is_null(current_block)){
		printf("| %d\t", counter);
		printf("| %p\t", current_block);
		printf("| %d\t", current_block->size);
		printf("| %p\t", current_block->next);
		
		if (current_block->next == NULL){
			printf("\t\t");
		}

		if (block_is_free(current_block)){
			printf("| FREE\t\t|");
		}
		else{
			printf("| OCCUPIED\t|");
		}

		printf("\n");

		counter++;
		current_block = block_next_neighbour(current_block);
	}
	printf("|-------+-----------------------+-------+-----------------------+---------------|\n");
}

// Return previous neighbour of given block if it exists. Returns null if it does not exist. 
// A neighbour can be free or in use. 
struct mem_control_block* block_previous_neighbour(struct mem_control_block * block){
	
	if (block_is_null(block)){
		// Given block can't be null
		return NULL;
	}

	if (block == managed_memory_start){
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
	printf("ERROR: Unable to find previous neighbour of block %p\n", block);
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

	// Declare variable to hold chosen block. 
	struct mem_control_block* chosen_block = (struct mem_control_block*)0;

	// Iterate through all free blocks, and choose the first with enough space
	struct mem_control_block* current_block = free_list_start;
	int split_chosen_block = 1;
	while (!block_is_null(current_block)){
		if (current_block->size >= numbytes){
			chosen_block = current_block;
			split_chosen_block = (int)(current_block->size - numbytes - sizeof(struct mem_control_block)) > 0;
			break;
		}
		current_block = block_next_free_block(current_block);
	}

	// Check that we found a block
	if (block_is_null(chosen_block)){
		printf("\nERROR: Unable to find a suitable block for allocating\n");
		return (void *)0;
	}


	// Save original attributes of chosen block before we start changing things
	int chosen_block_size = chosen_block->size;
	struct mem_control_block* chosen_block_next = chosen_block->next;


	// A suitable block has been chosen. 
	if (split_chosen_block){
		// Split the chosen block into two new blocks, one occupied and one free. 
		struct mem_control_block* occupied_block = chosen_block;	// Same start as chosen block
		struct mem_control_block* free_block = ((void*)occupied_block) + total_block_size;	// Starts where occupied block ends

		// Update size and next attributes
		occupied_block->size = numbytes;
		occupied_block->next = free_block;
		
		free_block->size = chosen_block_size - occupied_block->size - sizeof(struct mem_control_block);
		free_block->next = chosen_block_next;

		// Check if chosen block was the first free block
		if (chosen_block == free_list_start){
			// Chosen block was the fist free block.  
			// Free list needs to be updated to point to the new free block. (Not the occupied one)
			free_list_start = free_block;
		}

		// Make sure the previous block also points to the free block
		struct mem_control_block* free_block_previous_neighbour = block_previous_neighbour(free_block);
		if (free_block_previous_neighbour != NULL){  // In case prev doesn't exist.
			free_block_previous_neighbour->next = free_block;
		}
	}
	else{
		// Special case where we have to use all available space of the chosen block, 
		// which means we don't split it. 


		// Check if chosen block was the first free block
		if (chosen_block == free_list_start){
			// Chosen block was the fist free block, which is now occupied. 
			// Update the free list start to point to the next free block
			free_list_start = chosen_block->next;
		}

		// Make sure the previous block also points to the next free block
		struct mem_control_block* previous_neighbour = block_previous_neighbour(chosen_block);
		if (previous_neighbour != NULL){  // In case prev doesn't exist.
			previous_neighbour->next = chosen_block->next;
		}
	}

	// Return pointer to allocated memory.
	return chosen_block;
}

// Helper function for the free function.
struct mem_control_block * get_next(struct mem_control_block * block){
	// Return next control block. 
	// TODO: return null if no next block exists
	return (struct mem_control_block *)((void *)block + sizeof(struct mem_control_block) + block->size);
}

void myfree(void *firstbyte) {

  	// First, we need to look for adjacent free blocks, in case we need to combine.
  	// Create pointer to first free control block. 
  
  	struct mem_control_block *block_to_free = (struct mem_control_block *)firstbyte;
  	
	// Find neighbour blocks
	struct mem_control_block *next_neighbour = block_next_neighbour(block_to_free);
	struct mem_control_block *previous_neighbour = block_previous_neighbour(block_to_free);

	// Determine if neighbours are free
	int next_is_free = next_neighbour != NULL && block_is_free(next_neighbour);
	int previous_is_free = previous_neighbour != NULL && block_is_free(previous_neighbour);
	
    printf("next_is_free: %d\n", next_is_free);
	printf("previous_is_free: %d\n", previous_is_free);

	// CASE 1: Both neighbours are free
	if (previous_is_free && next_is_free){
		printf("This is case 1\n");
		struct mem_control_block *combined_block = previous_neighbour;	// Start at previous neighbour

		// Set size and next attributes 
		combined_block->size = previous_neighbour->size + block_to_free->size + next_neighbour->size + 2 * sizeof(struct mem_control_block);
		combined_block->next = next_neighbour->next;

		// Update next pointer of the combined block's previous free block
		struct mem_control_block *combined_block_previous_free_neighbour = block_previous_free_block(previous_neighbour);
		if (combined_block_previous_free_neighbour != NULL){
			combined_block_previous_free_neighbour->next = combined_block;
		}

		// Check if combined block is the first free block
		if (combined_block < free_list_start){
			free_list_start = combined_block;
		}
	}
  
    // CASE 2: Previous free, next occupied
    else if (previous_is_free && !next_is_free){
		printf("This is case 2\n");

		struct mem_control_block *combined_block = previous_neighbour;	// Start at previous neighbour

		// Set size and next attributes 
		combined_block->size = previous_neighbour->size + block_to_free->size + sizeof(struct mem_control_block);
		combined_block->next = previous_neighbour->next;

		// Update next pointer of the combined block's previous free block
		struct mem_control_block *combined_block_previous_free_neighbour = block_previous_free_block(previous_neighbour);
		if (combined_block_previous_free_neighbour != NULL){
			combined_block_previous_free_neighbour->next = combined_block;
		}

		// Check if combined block is the first free block
		if (combined_block < free_list_start){
			free_list_start = combined_block;
		}
    }

	// CASE 3: Previous occupied, next free
	else if (!previous_is_free && next_is_free){
		printf("This is case 3\n");

		struct mem_control_block *combined_block = block_to_free;	// Start at block to free

		// Set size and next attributes 
		combined_block->size = block_to_free->size + next_neighbour->size + sizeof(struct mem_control_block);
		combined_block->next = next_neighbour->next;

		// Update next pointer of the combined block's previous free block
		struct mem_control_block *combined_block_previous_free_neighbour = block_previous_free_block(previous_neighbour);
		if (combined_block_previous_free_neighbour != NULL){
			combined_block_previous_free_neighbour->next = combined_block;
		}

		// Check if combined block is the first free block
		if (combined_block < free_list_start){
			free_list_start = combined_block;
		}
  	}

    // CASE 4: No neighbours are free
  	else if (!previous_is_free && !next_is_free){
		printf("This is case 4\n");

		struct mem_control_block *combined_block = block_to_free;	// Start at block to free

		// Set next attribute 
		combined_block->next = block_next_free_block(block_to_free);

		// Update next pointer of the combined block's previous free block
		struct mem_control_block *combined_block_previous_free_neighbour = block_previous_free_block(previous_neighbour);
		if (combined_block_previous_free_neighbour != NULL){
			combined_block_previous_free_neighbour->next = combined_block;
		}

		// Check if combined block is the first free block
		if (combined_block < free_list_start){
			free_list_start = combined_block;
		}
  	}
  	else {
    	printf("ERROR: Unable to free block starting on address %p", firstbyte);
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

void mymalloc_test_with_20_bytes(){
	
	// Print all blocks to see our initial state
	printf("We should start with only one block of free memory.");
	block_print_all();

	printf("\nAllocating 20 bytes of memory. This is not a multuple of 8, so we should actually expect 24 bytes to be allocated. \n");
	mymalloc(20);


	printf("After allocating, we should now have one occupied block and one free block.");
	block_print_all();
}

void mymalloc_test_with_65_kilobytes(){
	
	// Print all blocks to see our initial state
	printf("We should start with only one block of free memory.");
	block_print_all();

	printf("\nAllocating 65 kB of memory. The total memory size is 64kB, so it should not be space for 65 kB. Therefore we expect no memory to be allocated, and an error to be printed. \n");
	mymalloc(65*1024);

	printf("After allocating, we should now still have only one free block.");
	block_print_all();
}

// Try to allocate the entire size of the first block. 
void mymalloc_test_with_first_block_size(){
	
	// Print all blocks to see our initial state
	printf("We should start with only one block of free memory.");
	block_print_all();

	int size = ((struct mem_control_block*)managed_memory_start)->size;

	printf("\nAllocating %i bytes of memory (block_0->size). ", size);
	printf("This is in other words the entire size of the free block. ");
	printf("Therefore we expect the entire size of the free block to be allocated, and we should only have one block after allocating.  \n");
	
	mymalloc(size);


	printf("After allocating, we should now have one occupied block. ");
	block_print_all();
}

int main(int argc, char **argv) {
    // Choose test to run by adjusting comments. 

	// mymalloc_test_with_20_bytes();
	// mymalloc_test_with_65_kilobytes();
	mymalloc_test_with_first_block_size();

    return 0;
}


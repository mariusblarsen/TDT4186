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

void block_combine_free_blocks(struct mem_control_block * block1, struct mem_control_block * block2){

	// Blocks can't be null
	if (block_is_null(block1) || block_is_null(block2)){
		printf("ERROR: Unable to combine free blocks. One of them is null. \n");
		printf("Block1: %p\n", block1);
		printf("Block2: %p\n", block2);
		return;
	}

	// Both blocks must be free
	if (!block_is_free(block1) || !block_is_free(block2)){
		printf("ERROR: Unable to combine free blocks. One of them is not free. \n");
		printf("Block1 free: %i\n", block_is_free(block1));
		printf("Block2 free: %i\n", block_is_free(block2));
		return;
	}

	// Blocks must be neighbours
	if (block_next_neighbour(block1) != block2){
		printf("ERROR: Unable to combine free blocks. Given blocks are not neighbours. \n");
		printf("Block1: %p\n", block1);
		printf("Block2: %p\n", block2);
		return;
	}

	// Parameters have passed validation, combine the blocks. 
	block1->size = block1->size + block2->size + sizeof(struct mem_control_block);
	block1->next = block2->next;
}

// Search for two free neighbour blocks. Return the first block if any exists, null pointer oterwise. 
struct mem_control_block* block_find_two_free_neighbours(){
	struct mem_control_block* current_block = (struct mem_control_block*)managed_memory_start;
	
	// Iterate through all blocks
	while(!block_is_null(current_block)){
		// Find next neighbour block
		struct mem_control_block* next_block = block_next_neighbour(current_block);
		if (block_is_free(current_block) && block_is_free(next_block)){
			// Both current and next block is free, return current block
			return current_block;
		}
		// Continue and check next block
		current_block = block_next_neighbour(current_block);
	}

	// No free blocks are found, return null pointer
	return NULL;
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
	// Cast parameter to block pointer
	struct mem_control_block* block = (struct mem_control_block*)firstbyte;

	// Given block can't be null
	if (block_is_null(block)){
		printf("ERROR: Unable to free block, given block is null (%p)", block);
		return;
	}

	struct mem_control_block* previous_free_block = block_previous_free_block(block);
	if (!block_is_null(previous_free_block)){
		// Update previous free block to point to this block
		block->next = previous_free_block->next;
		previous_free_block->next = block; 
	}
	else{
		// No previous free blocks, which means this will be the new first free block
		block->next = free_list_start;
		free_list_start = block;
	}

	// The given block is now free. 
	// Now we might have adjacent free blocks in our memory, which can be combined into larger blocks. 
	
	struct mem_control_block* free_block_with_free_neighbour = block_find_two_free_neighbours();
	while(!block_is_null(free_block_with_free_neighbour)){
		// Combine the two free blocks
		block_combine_free_blocks(free_block_with_free_neighbour, block_next_neighbour(free_block_with_free_neighbour));

		// Continue to next
		free_block_with_free_neighbour = block_find_two_free_neighbours();
	}



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


void myfree_test_with_one_occupied_one_free(){
	// Allocate new block
	struct mem_control_block* allocated_block = mymalloc(64);
	
	printf("We should start with one block of size %i bytes, and one free block with the rest of memory.", allocated_block->size);
	block_print_all();

	// Free the allocated block
	myfree(allocated_block);

	printf("\nThe allocated block is then freed, which should result in one remaining block, with the entire memory range as the size.");
	block_print_all();
}

void myfree_test_with_one_occupied(){
	// Initalize to create the first block
	if (has_initialized == 0) {
		mymalloc_init();
	}

	// Retrieve the first block
	struct mem_control_block* first_block = (struct mem_control_block*)managed_memory_start;
	int size = first_block->size;

	mymalloc(size);

	printf("We should start with one block of size %i bytes, which is occupied.", size);
	block_print_all();

	// Free the allocated block
	myfree(first_block);

	printf("\nThe block is then freed, which should result in one remaining free block, with the entire memory range as the size.");
	block_print_all();
}

void myfree_test_with_previous_free_next_occupied(){
	// Initalize to create the first block
	if (has_initialized == 0) {
		mymalloc_init();
	}

	// Retrieve the first block
	struct mem_control_block* first_block = (struct mem_control_block*)managed_memory_start;
	int size = first_block->size;

	// Allocate three blocks of memory
	struct mem_control_block* block1 = mymalloc(64);
	struct mem_control_block* block2 = mymalloc(64);
	int last_block_size = block_next_free_block(block2)->size;
	struct mem_control_block* block3 = mymalloc(last_block_size);

	// Free the first block to get the desired initial state
	myfree(block1);

	printf("We should start with the first block beeing free, and the second and third occupied.");
	block_print_all();

	// Free the second block
	myfree(block2);

	printf("\nThe second block is then freed, which should result in one remaining free block at the start, followed by one occupied.");
	block_print_all();
}

void myfree_test_with_previous_occupied_next_occupied(){
	// Initalize to create the first block
	if (has_initialized == 0) {
		mymalloc_init();
	}

	// Retrieve the first block
	struct mem_control_block* first_block = (struct mem_control_block*)managed_memory_start;
	int size = first_block->size;

	// Allocate three blocks of memory
	struct mem_control_block* block1 = mymalloc(64);
	struct mem_control_block* block2 = mymalloc(64);
	int last_block_size = block_next_free_block(block2)->size;
	struct mem_control_block* block3 = mymalloc(last_block_size);

	printf("We should start with three occupied blocks.");
	block_print_all();

	// Free the second block
	myfree(block2);

	printf("\nThe second block is then freed, which should not lead to any combining, resulting in one free block with occupied neighbours.");
	block_print_all();
}

void myfree_test_with_previous_free_next_free(){
	// Initalize to create the first block
	if (has_initialized == 0) {
		mymalloc_init();
	}

	// Retrieve the first block
	struct mem_control_block* first_block = (struct mem_control_block*)managed_memory_start;
	int size = first_block->size;

	// Allocate three blocks of memory
	struct mem_control_block* block1 = mymalloc(64);
	struct mem_control_block* block2 = mymalloc(64);
	int last_block_size = block_next_free_block(block2)->size;
	struct mem_control_block* block3 = mymalloc(last_block_size);

	// Free first and third block
	myfree(block1);
	myfree(block3);

	printf("We should start with one occupied block with two free neighbours.");
	block_print_all();

	// Free the second block
	myfree(block2);

	printf("\nThe second block is then freed, which should result in only one free block.");
	block_print_all();
}

void myfree_test_with_previous_occupied_next_free(){
	// Initalize to create the first block
	if (has_initialized == 0) {
		mymalloc_init();
	}

	// Retrieve the first block
	struct mem_control_block* first_block = (struct mem_control_block*)managed_memory_start;
	int size = first_block->size;

	// Allocate three blocks of memory
	struct mem_control_block* block1 = mymalloc(64);
	struct mem_control_block* block2 = mymalloc(64);
	int last_block_size = block_next_free_block(block2)->size;
	struct mem_control_block* block3 = mymalloc(last_block_size);

	// Free the third block
	myfree(block3);

	printf("We should start with two occupied blocks, followed by one free.");
	block_print_all();

	// Free the second block
	myfree(block2);

	printf("\nThe second block is then freed, which should result in one occupied block, followed by one free block.");
	block_print_all();
}

int main(int argc, char **argv) {
    // Choose test to run by adjusting comments. 

	// mymalloc_test_with_20_bytes();
	// mymalloc_test_with_65_kilobytes();
	// mymalloc_test_with_first_block_size();

	// myfree_test_with_one_occupied_one_free();
	// myfree_test_with_one_occupied();
	// myfree_test_with_previous_free_next_occupied();
	// myfree_test_with_previous_occupied_next_occupied();
	// myfree_test_with_previous_free_next_free();
	myfree_test_with_previous_occupied_next_free();
    return 0;
}


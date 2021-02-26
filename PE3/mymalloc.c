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

void *mymalloc(long numbytes) {
	if (has_initialized == 0) {
		mymalloc_init();
	}
	// make sure numbytes is devisible by 8, for correct memory allocation.
	while(numbytes%8 != 0){
		numbytes++;
	}

	// Calculte the size required by data and metadata
	long insert_data_size = numbytes + sizeof(struct mem_control_block);

	// Create pointer to first free control block. 
	struct mem_control_block *m;
	m = free_list_start;
	
	// Pointer to previous block, to move its next-pointer.
	struct mem_control_block *prev_m = (struct mem_control_block *)0;

	// Look for available space. Linked list style.
	while(m->size < insert_data_size){
		// We dont have room, go to next
		prev_m = m;
		m = m->next;
		// Next-pointer is NULL - no hit
		if(m == (struct mem_control_block *)0){
		return (void *)0;
		}
  	}


	// Now we have pointer to control block at start of free block.
	// Find pointer to the start of the new free area.
	void *start_of_free = (void *)m;
	start_of_free += insert_data_size;

	// At the start of the new free area, make a new control block. This controls the new free block.
	struct mem_control_block *new_m = (struct mem_control_block *)start_of_free;
	new_m->size = m->size - insert_data_size - sizeof(struct mem_control_block);
	new_m->next = m->next;

	// In case m was the first element in free list,
	// Free list needs to be updated to point to new_m, the new free block.
	if (m == free_list_start){
		free_list_start = new_m;
	}  

	// Make sure the previous control also points to the free block
	if (prev_m){  // In case prev_m doesn't exist.
		prev_m->next = m->next;
	}

	// Update size of m to numbytes.
	m->size = numbytes;
	// Also, m->next should point to the new_m, as its the first next free block.
	m->next = new_m;

	// Return pointer to allocated memory.
	return m;

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


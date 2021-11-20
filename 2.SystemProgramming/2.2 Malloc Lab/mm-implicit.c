/*
 * mm-implicit.c - an empty malloc package
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 *
 * @id : 201601989
 * @name : 김진섭
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif


/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
#define DSIZE 8 //double
#define WSIZE 4 //single
#define CHUNKSIZE (1<<12)
#define MAX(x,y) ((x) > (y) ? (x):(y))

#define PACK(size, alloc) ((size)|(alloc))

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(p) (((size_t)(p) + (ALIGNMENT-1)) & ~0x7)
#define GET(p) (*(unsigned int*)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define HDRP(bp) ((char *)(bp) -WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp))-DSIZE)
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char*)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char*)(bp) - DSIZE)))

/*그 외 구 현*/
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void *heap_listp;
static void *heap_next;

static void *extend_heap(size_t words){
	char *bp;
	size_t size;
	size = (words %2) ? (words+1) * WSIZE : words* WSIZE;
	bp = mem_sbrk(size);
	if((long)bp == -1 ){
		return NULL;
	}
	PUT(HDRP(bp),PACK(size,0));
	PUT(FTRP(bp),PACK(size,0));
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1));
	return coalesce(bp);
}

static void *coalesce(void *bp){
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));

	if(prev_alloc && next_alloc){
		return bp;
	}
	else if(prev_alloc && !next_alloc){
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp),PACK(size,0));
		PUT(FTRP(bp),PACK(size,0));
	}
	else if(!prev_alloc && next_alloc){
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
		bp = PREV_BLKP(bp);
	}
	else{
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)),PACK(size,0));
		bp = PREV_BLKP(bp);
	}
	heap_next = bp;
	return bp;
}

static void place(void *bp, size_t asize){
	size_t csize = GET_SIZE(HDRP(bp));
	if((csize - asize) >= (2*DSIZE)){
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(csize-asize,0));
		PUT(FTRP(bp), PACK(csize-asize,0));
	}
	else{
		PUT(HDRP(bp), PACK(csize,1));
		PUT(FTRP(bp), PACK(csize,1));
	}
}

static void *find_fit(size_t asize){
	for(; GET_SIZE(HDRP(heap_next)) > 0; heap_next = NEXT_BLKP(heap_next)){
		if(!GET_ALLOC(HDRP(heap_next)) && (asize <= GET_SIZE(HDRP(heap_next)))){
			return heap_next;
		}
	}
	return NULL;
}
/*
 * Initialize: return -1 on error, 0 on success.
 */
int mm_init(void) {
	heap_listp = mem_sbrk(4*WSIZE);
	if(heap_listp == NULL){
		return -1;
	}
	PUT(heap_listp, 0); // Prologue                      
	PUT(heap_listp +(1*WSIZE),PACK(DSIZE,1)); //header
	PUT(heap_listp +(2*WSIZE),PACK(DSIZE,1)); //footer
	PUT(heap_listp +(3*WSIZE),PACK(0,1)); //EPILOGUE
	heap_listp += (2*WSIZE);
	heap_next = heap_listp;

	if(extend_heap(CHUNKSIZE/WSIZE) == NULL){
		return -1;
	}
    return 0;
}

/*
 * malloc
 */
void *malloc (size_t size) {
	size_t asize;
	size_t extendsize;
	char *bp;

	if(size == 0){
		return NULL;
	}

	if(size <= DSIZE){
		asize = 2*DSIZE;
	}
	else{
		asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
	}
	bp = find_fit(asize);
	if(bp !=NULL){
		place(bp, asize);
		return bp;
	}
	
	extendsize = MAX(asize,CHUNKSIZE);
	if((bp = extend_heap(extendsize/WSIZE)) == NULL){
		return NULL;
	}
	place(bp, asize);
	return bp;
}

/*
 * free
 */
void free(void *ptr) {
	if(ptr == 0)
		return;
	size_t size = GET_SIZE(HDRP(ptr));
	PUT(HDRP(ptr), PACK(size,0));
	PUT(FTRP(ptr), PACK(size,0));
	coalesce(ptr);
}

/*
 * realloc - you may want to look at mm-naive.c
 */
void *realloc(void *oldptr, size_t size) {
	size_t oldsize;
	void *newptr;

	if(size == 0){
		free(oldptr);
		return 0;
	}
	if(oldptr == NULL){
		return malloc(size);
	}
	newptr = malloc(size);

	if(!newptr){
		return 0;
	}
	oldsize = GET_SIZE(oldptr);
	if(size < oldsize){
		oldsize = size;
	}
	memcpy(newptr, oldptr, oldsize);
	free(oldptr);
    return newptr;
}

/*
 * calloc - you may want to look at mm-naive.c
 * This function is not tested by mdriver, but it is
 * needed to run the traces.
 */
void *calloc (size_t nmemb, size_t size) {
    return NULL;
}


/*
 * Return whether the pointer is in the heap.
 * May be useful for debugging.
 */
static int in_heap(const void *p) {
    return p < mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * Return whether the pointer is aligned.
 * May be useful for debugging.
 */
static int aligned(const void *p) {
    return (size_t)ALIGN(p) == (size_t)p;
}

/*
 * mm_checkheap
 */
void mm_checkheap(int verbose) {
}

/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.(수정필요)
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "team 4",
    /* First member's full name */
    "Jisu Heo",
    /* First member's email address */
    "jisu@jungle.com",
    /* Second member's full name (leave blank if none) */
    "BG Shin",
    /* Second member's email address (leave blank if none) */
    "bgShin@jungle.com"};

// Basic constants and macros for manipulating the free list.
// 가용 블럭 리스트를 활용하기 위한 기본적인 상수와 매크로 함수 정의
#define WSIZE 4             /* Word and header/footer size (bytes) */
#define DSIZE 8             /* Double word size (bytes) */
#define CHUNKSIZE   (1<<12) /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
// #define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp) - DSIZE))
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)


/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Global variables */
static char *heap_listp = 0;  /* Pointer to first block */  


void *coalesce(void *bp);
static void place(void *bp, size_t asize);
void *mm_malloc(size_t size);
void mm_free(void *bp);
static void *extend_heap(size_t words);
static void *find_fit(size_t asize);
static void mm_copy(void *new, void *old, size_t n);

extern int mm_init(void);
extern void *mm_malloc (size_t size);
extern void mm_free (void *ptr);

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // printf("mm_init start------------------------\n");
    // 빈 힙을 새로 만들기
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
    {
        return -1;
    }
    
    PUT(heap_listp, 0);
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));
    heap_listp += (2*WSIZE);

    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) 
    {
        return -1;
    }

    // printf("mm_init done ------------------------\n");
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    // printf("mm_malloc start------------------------\n");
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;      

    if (size == 0)
    {
        // printf("1) mm_malloc done ------------------------\n");
        return NULL;
    }
	
    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
    {                                          
	    asize = 2*DSIZE;                                        
    }
    else
    {
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
    }
	
    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) 
    {
	    place(bp, asize);
        // printf("2) mm_malloc done ------------------------\n");                  
	    return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
    {
        // printf("3) mm_malloc done ------------------------\n");
        return NULL;
    }
        
    place(bp, asize);
    // printf("4) mm_malloc done ------------------------\n");
    return bp;
}    


/* extend_heap*/
static void *extend_heap(size_t words) 
{
    // printf("extend_heap start ------------------------\n");
    char *bp;
    size_t size;

    size = (words % 2)? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1) 
    {
        return NULL;
    }

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    // 새 에필로그 블록을 할당
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    // printf("extend_heap done ------------------------\n");
    return coalesce(bp);
}


/* find_fit
 * 응용 프로그램에서 요청한 사이즈에 맞는 블럭이 있는지 first-fit방식으로 찾는다.
 */
static void *find_fit(size_t asize)
{
    // printf("find_fit start ------------------------\n");
    void *bp;

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {   
        // printf("\n");
        // printf("for ----------------------------------------------\n");
        // printf("bp = heap_listp: %p\n", bp);
        // printf("bp = NEXT_BLKP: %p\n", bp);
        // printf("HDRP(bp): %p\n", HDRP(bp));
        // printf("!GET_ALLOC(HDRP(bp)): %d\n", !GET_ALLOC(HDRP(bp)));
        // printf("asize: %d\n", asize);
        // printf("GET_SIZE(HDRP(bp)): %d\n", GET_SIZE(HDRP(bp)));
        // printf("asize <= GET_SIZE(HDRP(bp)): %d\n", asize <= GET_SIZE(HDRP(bp)));
        // printf("!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))): %d\n", !GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))));
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
        {
            // printf("1) find_fit done ------------------------\n");
            return bp;
        }
    }
    // printf("2) find_fit done ------------------------\n");
    return NULL;
}

/* place */
static void place(void *bp, size_t asize)
{
    // printf("place start ------------------------\n");
    size_t csize = GET_SIZE(HDRP(bp));
    
    if ((csize - asize) >= (2*DSIZE)) 
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
    }
    else
    {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
    // printf("place done ------------------------\n");
}


/* mm_free - Freeing a block does nothing.
 * 블럭 해제: 더 이상 데이터를 담지 않는 블럭들을 비할당 상태로 만든다.
 */
void mm_free(void *bp)
{
    // printf("mm_free start ------------------------\n");
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

    coalesce(bp);
    // printf("mm_free done ------------------------\n");
}


/* mm_coalesce - Determines if the previous and next blocks are allocated.
 * Depending on the cases, the current block may be coalesced.
 * 블럭 병합: 현재 블럭의 앞뒤 블럭이 가용상태(free)인지 확인하고 병합 가능한 경우 두 블럭을 병합한다.
 * 1) 앞뒤 블럭 모두 할당된 경우(앞뒤 블럭 모두 가용 상태가 아닌 경우)
 * 2) 뒤의 블럭만 가용한 경우
 * 3) 앞의 블럭만 가용한 경우
 * 4) 앞뒤 블럭 모두 가용할 경우(앞뒤 블럭 모두 할당되지 않은 경우)
 */
void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); // 현재 블럭의 바로 앞 블럭이 병합가능한가를 확인
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); // 현재 블럭의 바로 뒤 블럭이 병합가능한가를 확인
    size_t size = GET_SIZE(HDRP(bp));
    // printf("FTRP(PREV_BLKP(bp)): %p\n", FTRP(PREV_BLKP(bp)));
    // printf("HDRP(NEXT_BLKP(bp)): %p\n", HDRP(NEXT_BLKP(bp)));
    // printf("GET_ALLOC(FTRP(PREV_BLKP(bp))): %d\n", GET_ALLOC(FTRP(PREV_BLKP(bp))));
    // printf("GET_ALLOC(HDRP(NEXT_BLKP(bp))): %d\n", GET_ALLOC(HDRP(NEXT_BLKP(bp))));
    // printf("GET_SIZE(HDRP(bp)): %d\n", size);
    
    // printf("coalesce start ------------------------\n");
    /*
     * 1) 앞뒤 블럭 모두 이미 할당된 경우 
     */
    if (prev_alloc && next_alloc) // (1 && 1)
    {
        // printf("(1) if coalesce ------------------------\n");
        // printf("1) coalesce done ------------------------\n");
        return bp;
    }
    
    /*
     * 2) 뒤의 블럭만 가용한 경우
     */
    else if (prev_alloc && !next_alloc) // (1 && !0)
    {
        // printf("(2) if coalesce ------------------------\n");
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    /*
     * 3) 앞의 블럭만 가용한 경우
     */
    else if (!prev_alloc && next_alloc) // (!0 && 1)
    {
        // printf("(3) if coalesce ------------------------\n");
        // TODO: size += GET_SIZE(FTRP(PREV_BLKP(bp))); 로 테스트해보기
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        // TODO: PUT(HDRP(bp), PACK(size, 0)); 도 해야하는 거 아닌지
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } 
    
    /*
     * 4) 앞뒤 블럭 모두 가용할 경우
     */
    else // (!0 && !0)
    {
        // printf("(4) if coalesce ------------------------\n");
        // TODO: 둘이 같지 않은가 size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        // size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + GET_SIZE(FTRP(PREV_BLKP(bp)));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    
    // printf("2) coalesce done ------------------------\n");
    return bp;
}


/*
 * mm_copy(A, B, N) - Copy N bytes of A to B.
 * 과제 설명서에는 직접 구현한 함수 외에는 어떤 C언어의 메모리 괄리 내장함수를 사용하지 말라고 
 * 하고 있으므로 mm_realloc에 사용되는 memcpy를 직접 구현한다.
 */
static void mm_copy(void *new, void *old, size_t n)
{
    char *cnew = (char *) new;
    char *cold = (char *) old;
    
    for (int i = 0; i < n; i++) {
        cnew[i] = cold[i];
    } 

}


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
    {
        // printf("1) mm_realloc done ------------------------\n");
        return NULL;
    }
      
    copySize = GET_SIZE(HDRP(oldptr));
    
    if (size < copySize) 
    {
        copySize = size;
    }
      
    mm_copy(newptr, oldptr, copySize);
    mm_free(oldptr);
    // printf("2) mm_realloc done ------------------------\n");
    return newptr;
}

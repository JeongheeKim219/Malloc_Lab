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
#define CHUCKSIZE (1 << 12) /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(insigned int *)(p))
#define PUT(p, val) (*(insigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp) - DSIZE))

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp)-DSIZE)))

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
        return NULL;
    else
    {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}


/*
 * mm_coalesce - Determines if the previous and next blocks are allocated.
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

    /*
     * 1) 앞뒤 블럭 모두 이미 할당된 경우 
     */
    if (prev_alloc && next_alloc) // (!0 && !0)
    {
        return bp;
    }
    
    /*
     * 2) 뒤의 블럭만 가용한 경우
     */
    else if (prev_alloc && !next_alloc) // (1 && !0)
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    /*
     * 3) 앞의 블럭만 가용한 경우
     */
    else if (!prev_alloc && next_alloc) // (!0 && 1)
    {
        size += GET_SIZE(FTRP(PREV_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } 
    
    /*
     * 4) 앞뒤 블럭 모두 가용할 경우
     */
    else // (1 && 1)
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + GET_SIZE(FTRP(PREV_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    return bp;

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
        return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

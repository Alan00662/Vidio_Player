#ifndef  __SHARE_H_
#define  __SHARE_H_

#ifdef  __cplusplus
    extern "C" {
#endif

#include <stdint.h>

#define set_bit(x, bit) ((x) |= 1 << (bit))
#define clr_bit(x, bit) ((x) &= ~(1 << (bit)))
#define tst_bit(x, bit) ((x) & (1 << (bit)))
#define get_bits(val,x1,x2)   (((val)>>(x1))&((1<<((x2)-(x1)+1))-1))

#define array_size(array) (sizeof(array)/sizeof(*array))
#define ARRAY_SIZE(arr)   (sizeof(arr)/sizeof((arr)[0])) 

#ifndef  min
#define min(a, b) ((a)<(b)?(a):(b))
#endif

#ifndef  max
#define max(a, b) ((a)>(b)?(a):(b))
#endif

#ifndef  swap
#define  swap(a, b)  do{a+=b; b=a-b; a-=b;}while(0)
#endif

#define  likely(exp)    __builtin_expect(!!(exp), 1)
#define  unlikely(exp)  __builtin_expect(!!(exp), 0)


#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef  container_of
#define container_of(ptr, type, member) ((type *)((char *)ptr - offsetof(type,member)))
#endif


/*
 * This looks more complex than it should be. But we need to
 * get the type for the ~ right in round_down (it needs to be
 * as wide as the result!), and we want to evaluate the macro
 * arguments just once each.
 */
#define __round_mask(x, y) ((uint32_t)((y)-1))
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
#define round_down(x, y) ((x) & ~__round_mask(x, y))

/*
 * ffs - find first bit in word.
 * @word: The word to search
 *
 * Undefined if no bit exists, so code should check against 0 first.
 */
unsigned long __ffs(unsigned long word);
/*
 * fls - find last (most-significant) bit set
 * @x: the word to search
 *
 * This is defined the same way as ffs.
 * Note fls(0) = 0, fls(1) = 1, fls(0x80000000) = 32.
 */
unsigned long __fls(unsigned long x);		

/*
 * round up to nearest power of two
 */
static inline
unsigned long __roundup_pow_of_two(unsigned long n)
{
    return 1UL << __fls(n - 1);
}

/*
 * round down to nearest power of two
 */
static inline
unsigned long __rounddown_pow_of_two(unsigned long n)
{
    return 1UL << (__fls(n) - 1);
}
#define roundup_pow_of_two(n) __roundup_pow_of_two(n)
#define rounddown_pow_of_two(n) __rounddown_pow_of_two(n)

void bubbleSort(int *arr, int n);

#ifdef  __cplusplus
}  
#endif

#endif



#include "libc.h"


/* 这些库函数移植于glibc与dietlibc */

//==string.h=============================================================================
#ifndef USE_FAST_LIB
void *memset(void * dst, int s, size_t count) {
    register char * a = dst;
    count++;	/* this actually creates smaller code than using count-- */
    while (--count)
	*a++ = s;
    return dst;
}

#else
void *memset(void *dstpp, int c, size_t len)
{
	long int dstp = (long int) dstpp;

	if (len >= OP_T_THRES)
    {
		size_t xlen;
		op_t cccc;

		cccc = (unsigned char) c;
		cccc |= cccc << 8;
		cccc |= cccc << 16;
		if (OPSIZ > 4)
			/* Do the shift in two steps to avoid warning if long has 32 bits.  */
			cccc |= (cccc << 16) << 16;

		/* There are at least some bytes to set.
		No need to test for LEN == 0 in this alignment loop.  */
		while (dstp % OPSIZ != 0)
		{
			((byte *) dstp)[0] = c;
			dstp += 1;
			len -= 1;
		}

		/* Write 8 `op_t' per iteration until less than 8 `op_t' remain.  */
		xlen = len / (OPSIZ * 8);
		while (xlen > 0)
		{
			((op_t *) dstp)[0] = cccc;
			((op_t *) dstp)[1] = cccc;
			((op_t *) dstp)[2] = cccc;
			((op_t *) dstp)[3] = cccc;
			((op_t *) dstp)[4] = cccc;
			((op_t *) dstp)[5] = cccc;
			((op_t *) dstp)[6] = cccc;
			((op_t *) dstp)[7] = cccc;
			dstp += 8 * OPSIZ;
			xlen -= 1;
		}
		len %= OPSIZ * 8;

		/* Write 1 `op_t' per iteration until less than OPSIZ bytes remain.  */
		xlen = len / OPSIZ;
		while (xlen > 0)
		{
			((op_t *) dstp)[0] = cccc;
			dstp += OPSIZ;
			xlen -= 1;
		}
		len %= OPSIZ;
    }

	/* Write the last few bytes.  */
	while (len > 0)
    {
		((byte *) dstp)[0] = c;
		dstp += 1;
		len -= 1;
    }

  return dstpp;	
}
#endif
void *u_memset(void* s, int c, size_t n) __attribute__((alias("memset")));


#ifndef  USE_FAST_LIB
void *memcpy (void *dst, const void *src, size_t n)
{
    void           *res = dst;
    unsigned char  *c1, *c2;

    c1 = (unsigned char *) dst;
    c2 = (unsigned char *) src;
    while (n--) *c1++ = *c2++;
    return (res);	
}

#else

/* 对于现代大多数CPU, 源地址与目的地址即使都不往机器字长对齐,
 * 也能照样能按机器字长拷贝数据(只不过效率不对齐时低,但还是比单字节拷贝效率高).
 * 对于一些老的CPU(比如S3C2440),如果源地址与目的地址不按机器字长对齐,
 * 则不能按机器字长拷贝数据, 此时只能2字节拷贝或者单字节拷贝. 
 * 为了兼顾新老CPU, 源地址与目的地址一律判断是佛按机器字长对齐(还是2字节对齐)
 */
void *memcpy (void *dst, const void *src, size_t n)
{
	unsigned long dstp = (unsigned long)dst;
	unsigned long srcp = (unsigned long)src;
	
	//学习上面的阈值为>OP_T_THRES的思想
	if(n > OP_T_THRES)
	{
		size_t xlen;

		/* There are at least some bytes to set.
		No need to test for LEN == 0 in this alignment loop.  */
		while (dstp % OPSIZ != 0)  //把目的地址向机器字长对齐
		{
			((byte *) dstp)[0] = ((byte *) srcp) [0];
			dstp += 1;
			srcp += 1;
			n -= 1;
		}
		
		if(srcp % OPSIZ == 0)  //源地址也按机器字长对齐了
		{                      /* No need to test for n==0 */
			/* Write 4 'op_t' per iteration until less than 4 `op_t' remain.  */
			xlen = n / (OPSIZ * 4);
			while(xlen--)
			{
				((op_t *) dstp)[0] = ((op_t *) srcp)[0];
				((op_t *) dstp)[1] = ((op_t *) srcp)[1];
				((op_t *) dstp)[2] = ((op_t *) srcp)[2];
				((op_t *) dstp)[3] = ((op_t *) srcp)[3];
				dstp += OPSIZ*4;
				srcp += OPSIZ*4;
			}
			n %= OPSIZ*4;  //现在数据剩余量
		}
		
		if(srcp % 2 == 0) //源地址2字节对齐了
		{                 /* No need to test for n==0 */
			xlen = n / (2*4);  
			while(xlen--)
			{
				((uint16_t *) dstp)[0] = ((uint16_t *) srcp)[0];
				((uint16_t *) dstp)[1] = ((uint16_t *) srcp)[1];
				((uint16_t *) dstp)[2] = ((uint16_t *) srcp)[2];
				((uint16_t *) dstp)[3] = ((uint16_t *) srcp)[3];
				dstp += 2*4;
				srcp += 2*4;				
			}
			n %= (2*4);  //现在数据剩余量, 最多剩余7字节, 直接用下面的字节拷贝就行
		}
	}
	
	while(n--)   /* No need to test for n==0 */
	{
		((byte *) dstp)[0] = ((byte *) srcp) [0];
		dstp += 1;
		srcp += 1;
	}

	return dst;
}
#endif
void *u_memcpy (void *dst, const void *src, size_t n) __attribute__((alias("memcpy")));

void *memmove(void *dst, const void *src, size_t count)
{
	char *a = dst;
	const char *b = src;
	if (src!=dst)
	{
		if (src>dst)
		{
			while (count--) *a++ = *b++;
		}
		else
		{
			a+=count-1;
			b+=count-1;
			while (count--) *a-- = *b--;
		}
	}
	return dst;
}
void *u_memmove (void *dst, const void *src, size_t n) __attribute__((alias("memmove")));


//==================memmem 暴力搜索算法=============================================
void *memmem_direct_search(const void* haystack, size_t hl, const void* needle, size_t nl)
{
	const char *phay = haystack;
	const char *pneed = needle;
	size_t i, j;
	
	if(hl < nl)       //size_t 是无符号类型,这个判断很重要,否则就会出BUG
	{	return NULL;	}
	
	for(i=0; i<= hl-nl; i++)
	{
		for(j=0; j<nl; j++)
		{
			if(phay[i+j] != pneed[j])
				break;
		}
		if(j >= nl)
			return (char *)haystack+i;
	}
	return NULL;
}

//===========================memmem KMP搜索算法=====================================
#define KMP_NEED_MAX_LEN  64
static void get_next(const char *str, int len, int *next)
{
	int i, k;
	next[0] = -1;
	k = -1;
	i = 0;
	while(i < len-1) {
		if(k==-1 || str[i]==str[k]) {
			i++;
			k++;
			next[i] = k;
		}
		else {
			k = next[k];
		}
	}
}


//在needle(几十字节) 与 haystack(几百字节) 都不大时, KMP算法还没有暴力搜索快
void *memmem_kmp_search(const void* haystack, size_t hl, const void* needle, size_t nl)
{
	int next[KMP_NEED_MAX_LEN];
	const char *phay = haystack;
	const char *pneed = needle;
	int i=0, j=0;
	
	get_next(pneed, nl, next);
	
	while(i<(int)hl && j<(int)nl) {
		if(j==-1 || phay[i]==pneed[j]) {
			i++;
			j++;
		}
		else {
			j = next[j];
		}
	}
	
	if(j >= (int)nl) {
		return (char *)haystack+i-nl;
	}
	else {
		return NULL;
	}
}

//一般haystack与needle都不大, 所以不使用KMP. 直接使用暴力搜索算法
void *memmem(const void* haystack, size_t hl, const void* needle, size_t nl)
{
//	if(nl < OP_T_THRES){  /* 直接使用暴力搜索 */
//		return memmem_direct_search(haystack, hl, needle, nl);
//	}
//	else{                 /* 借助memcmp实现 */
//		
//		return NULL;
//	}
	return memmem_direct_search(haystack, hl, needle, nl);
}
void *u_memmem (const void* haystack, size_t hl, const void* needle, size_t nl) __attribute__((alias("memmem")));



size_t strlen(const char *str)
{
	const char *s;
	for (s = str; *s; ++s)
		;
	return (s - str);
}
size_t u_strlen(const char *str) __attribute__((alias("strlen")));

size_t strnlen(const char *str, size_t count)
{
	const char *sc;
	for (sc = str; count-- && *sc != '\0'; ++sc)
		/* nothing */;
	return sc - str;
}
size_t u_strnlen(const char *str, size_t count) __attribute__((alias("strnlen")));

/*
 * Compare strings.
 */
int strcmp(const char *s1, const char *s2)
{
	while (*s1 == *s2++) {
		if (*s1++ == 0)
			return (0);
	}
	return (*(unsigned char *)s1 - *(unsigned char *)--s2);
}
int u_strcmp(const char *s1, const char *s2) __attribute__((alias("strcmp")));

int strncmp(const char *s1, const char *s2, size_t n)
{
	if (n == 0)
		return (0);
	do {
		if (*s1 != *s2++)
			return (*(unsigned char *)s1 - *(unsigned char *)--s2);
		if (*s1++ == 0)
			break;
	} while (--n != 0);
	return (0);
}
int u_strncmp(const char *s1, const char *s2, size_t n) __attribute__((alias("strncmp")));


char *strcpy(char *to, const char *from)
{
	char *save = to;

	for (; (*to = *from) != '\0'; ++from, ++to);
	return(save);
}
char *u_strcpy(char *to, const char *from) __attribute__((alias("strcpy")));

/*
 * Copy src to dst, truncating or null-padding to always copy n bytes.
 * Return dst.
 */
char *strncpy(char *dst, const char *src, size_t n)
{
	if (n != 0) {
		char *d = dst;
		const char *s = src;

		do {
			if ((*d++ = *s++) == 0) {
				/* NUL pad the remaining n-1 bytes */
				while (--n != 0)
					*d++ = 0;
				break;
			}
		} while (--n != 0);
	}
	return (dst);
}
char *u_strncpy(char *dst, const char *src, size_t n) __attribute__((alias("strncpy")));




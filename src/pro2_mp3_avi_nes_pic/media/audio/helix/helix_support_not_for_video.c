#include "helix_support.h"

/** ʵ��libhelix����ʹ�õ��ⲿ����
 **/

#include "libc.h"
#include "my_malloc.h"

void *helix_memcpy (void *dst, const void *src, size_t n)
{
	return u_memcpy(dst, src, n);
}

void *helix_memmove(void *dst, const void *src, size_t count)
{
	return u_memmove(dst, src, count);
}

void *helix_malloc(unsigned int size)
{
	return mem_malloc(size);
}

void helix_free(void *mem_ptr)
{
	mem_free(mem_ptr);
}





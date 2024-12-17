#ifndef  __HELIX_SUPPORT_H_
#define  __HELIX_SUPPORT_H_

#include <stddef.h>

/** @brief ���ļ��������ĺ�����helix���е��õĺ���, ��Ҫ�ڿ���ʵ����Щ����
 **/

extern void *helix_memcpy (void *dst, const void *src, size_t n);
extern void *helix_memmove(void *dst, const void *src, size_t count);
extern void *helix_malloc(unsigned int size);
extern void helix_free(void *mem_ptr);


#endif




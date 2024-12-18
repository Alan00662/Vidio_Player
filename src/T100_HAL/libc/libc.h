#ifndef  __LIBC_H_
#define  __LIBC_H_

#ifdef  __cplusplus
    extern "C" {
#endif

//编译器提供必要的信息
#include <stddef.h>   //define size_t
#include <inttypes.h>
#include <stdint.h>   //提供uint8_t int16_t等数据类型
#include <stdarg.h>   //可变函数参数的提取
#include <limits.h>   //提供long int等数据类型的大小


/* Type to use for aligned memory operations.
   This should normally be the biggest type supported by a single load
   and store.  */
#define	op_t	unsigned long int
#define OPSIZ	(sizeof (op_t))

/* Type to use for unaligned operations.  */
typedef unsigned char byte;	

#define OP_T_THRES	16


#define  USE_FAST_LIB

/* 这些库函数移植于glibc与dietlibc */	
//string.h
void *memset(void* s, int c, size_t n);
void *memcpy (void *dst, const void *src, size_t n);
void *memmove(void *dst, const void *src, size_t count);
//int  memcmp(const void *dst, const void *src, size_t count);   //还未实现
void *memmem_direct_search(const void* haystack, size_t hl, const void* needle, size_t nl);
void *memmem_kmp_search(const void* haystack, size_t hl, const void* needle, size_t nl);
void *memmem(const void* haystack, size_t hl, const void* needle, size_t nl); //直接使用暴力搜索

/* Keil ARMCC编译器不支持编译时打桩机制,与GCC编译器相比弱爆了 */
void *u_memset(void* s, int c, size_t n);
void *u_memcpy (void *dst, const void *src, size_t n);
void *u_memmove (void *dst, const void *src, size_t n);
void *u_memmem(const void* haystack, size_t hl, const void* needle, size_t nl);

size_t strlen(const char *str);
size_t strnlen(const char *str, size_t count);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
char *strcpy(char *to, const char *from);
char *strncpy(char *dst, const char *src, size_t n);

size_t u_strlen(const char *str);
size_t u_strnlen(const char *str, size_t count);
int u_strcmp(const char *s1, const char *s2);
int u_strncmp(const char *s1, const char *s2, size_t n);
char *u_strcpy(char *to, const char *from);
char *u_strncpy(char *dst, const char *src, size_t n);

//stdlib.h
//string convert to value functions,  the code copy from Android 4.4 bonic C library
unsigned long strtoul(const char *nptr, char **endptr, int base);
long strtol(const char *nptr, char **endptr, int base);
intmax_t strtoimax(const char *nptr, char **endptr, int base);
int  atoi(const char*  s);
long  atol(const char*  s);
long long strtoll(const char *nptr, char **endptr, int base);
uintmax_t strtoumax(const char *nptr, char **endptr, int base);
unsigned long long strtoull(const char *nptr, char **endptr, int base);

//Keil ARMCC编译器不支持编译时打桩机制, 得给函数令起一个名字
unsigned long u_strtoul(const char *nptr, char **endptr, int base);
long u_strtol(const char *nptr, char **endptr, int base);
intmax_t u_strtoimax(const char *nptr, char **endptr, int base);
int  u_atoi(const char*  s);
long  u_atol(const char*  s);
long long u_strtoll(const char *nptr, char **endptr, int base);
uintmax_t u_strtoumax(const char *nptr, char **endptr, int base);
unsigned long long u_strtoull(const char *nptr, char **endptr, int base);

//value convert to string  use sprintf() or snprintf() is OK 


//stdio.h
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
int snprintf(char * buf, size_t size, const char *fmt, ...);
int vsprintf(char *buf, const char *fmt, va_list args);
int sprintf(char * buf, const char *fmt, ...);
int vsscanf(const char * buf, const char * fmt, va_list args);
int sscanf(const char * buf, const char * fmt, ...);

/* Keil ARMCC编译器不支持编译时打桩机制,与GCC编译器相比弱爆了 */
int u_vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
int u_snprintf(char * buf, size_t size, const char *fmt, ...);
int u_sprintf(char * buf, const char *fmt, ...);
int u_vsscanf(const char * buf, const char * fmt, va_list args);
int u_sscanf(const char * buf, const char * fmt, ...);

//time.h

		
#ifdef  __cplusplus
}  
#endif		
		
#endif


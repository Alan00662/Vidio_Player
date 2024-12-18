#include "my_printf.h"
#include "usart.h"
#include "share.h"
#include "libc.h"

int my_printf(const char *fmt, ...)
{
	int len;
	char buff[256];   //不能太大,也不能太小
	va_list ap;
	
	va_start(ap, fmt);
	len = u_vsnprintf(buff, sizeof(buff), fmt, ap);  //len = strlen(buf); 不包括'\0'
	va_end(ap);

	uart1_write(buff, len);
	
	return len;
}


static inline void i2str(uint8_t val, char *dest)
{
    const char *charmap = "0123456789ABCDEF";

    *dest++ = charmap[get_bits(val, 4, 7)];
    *dest++ = charmap[get_bits(val, 0, 3)];
}
static inline const char *arr2str(const void *arr, int len, void *dest, int maxlen)
{
    const uint8_t *_arr = (const uint8_t *)arr;
    char *_dest = (char *)dest;

    while (len-- && maxlen > 0)
    {
        i2str(*_arr++, _dest);
        _dest += 2;
        *_dest++ = ' ';
        maxlen -= 3;
    }

    return (const char *)dest;
}

void my_printf_arr(const void *buff, int len, uint32_t start_addr)
{
	const char *pdat = buff;
	char temp[128];
	int  i, ch, idx, t_len;

	while(len > 0)
	{
		t_len = min(len, 16);
		len -= t_len;
		u_snprintf(temp, sizeof(temp), "%08X  ", start_addr);  //地址信息
		start_addr += t_len;
		idx = 10;
		arr2str(pdat, t_len, &temp[idx], sizeof(temp)-idx);
		if(t_len != 16) {
			u_memset(&temp[idx + t_len*3], ' ', (16-t_len)*3);
		}
		idx += 16*3;
		temp[idx++] = ' ';
		temp[idx++] = '|';
		for(i=0; i<t_len; i++) {
			ch = *pdat++;
			if(ch<0x20 || ch>0x7E)  //不可见字符
				temp[idx++] = '.';
			else
				temp[idx++] = (char)ch;
		}
		temp[idx++] = '|';
		temp[idx++] = '\n';
		temp[idx++] = '\r';
//		temp[idx++] = 0;        //end of string, no need

		uart1_write(temp, idx);
	}
}
//void printf_arr(const void *buff, int len, uint32_t start_addr) __attribute__((alias("my_printf_arr")));

//===========================================================================
int d_printf(const char *fmt, ...)
{
	int len;
	char buff[256];   //不能太大,也不能太小
	va_list ap;
	
	va_start(ap, fmt);
	len = u_vsnprintf(buff, sizeof(buff), fmt, ap);  //len = strlen(buf); 不包括'\0'
	va_end(ap);

//	uart3_write(buff, len);
	
	return len;
}

void d_printf_arr(const void *buff, int len, uint32_t start_addr)
{
	const char *pdat = buff;
	char temp[128];
	int  i, ch, idx, t_len;

	while(len > 0)
	{
		t_len = min(len, 16);
		len -= t_len;
		u_snprintf(temp, sizeof(temp), "%08X  ", start_addr);  //地址信息
		start_addr += t_len;
		idx = 10;
		arr2str(pdat, t_len, &temp[idx], sizeof(temp)-idx);
		if(t_len != 16) {
			u_memset(&temp[idx + t_len*3], ' ', (16-t_len)*3);
		}
		idx += 16*3;
		temp[idx++] = ' ';
		temp[idx++] = '|';
		for(i=0; i<t_len; i++) {
			ch = *pdat++;
			if(ch<0x20 || ch>0x7E)  //不可见字符
				temp[idx++] = '.';
			else
				temp[idx++] = (char)ch;
		}
		temp[idx++] = '|';
		temp[idx++] = '\n';
		temp[idx++] = '\r';
//		temp[idx++] = 0;        //end of string, no need

//		uart3_write(temp, idx);
	}	
}




#include "ctype.h"

/** ÒÆÖ²ÓÚdietlibc-0.34
 */

int isascii ( int ch ) 
{
    return (unsigned int)ch < 128u;
}


int isblank ( int ch )
{
    return ch == ' '  ||  ch == '\t';
}


int isalnum(int ch) {
  return (unsigned int)((ch | 0x20) - 'a') < 26u  ||
	 (unsigned int)( ch         - '0') < 10u;
}


int isalpha(int ch) {
  return (unsigned int)((ch | 0x20) - 'a') < 26u;
}

int __isdigit_ascii ( int ch );
int __isdigit_ascii ( int ch ) {
    return (unsigned int)(ch - '0') < 10u;
}
//int isdigit ( int ch ) __attribute__((weak,alias("__isdigit_ascii")));
int isdigit ( int ch ) __attribute__((alias("__isdigit_ascii")));


int __isspace_ascii ( int ch );
int __isspace_ascii ( int ch )
{
    return (unsigned int)(ch - 9) < 5u  ||  ch == ' ';
}
//int isspace ( int ch ) __attribute__((weak,alias("__isspace_ascii")));
int isspace ( int ch ) __attribute__((alias("__isspace_ascii")));


int isupper(int c) {
  unsigned char x=c&0xff;
  return (x>='A' && x<='Z') || (x>=192 && x<=222 && x!=215);
}


int islower(int c) {
  unsigned char x=c&0xff;
  return (x>='a' && x<='z') || (x>=223 && x!=247);
}


int toascii(int c) {
  return (c&0x7f);
}


int tolower(int ch) {
  if ( (unsigned int)(ch - 'A') < 26u )
    ch += 'a' - 'A';
  return ch;
}


int toupper(int ch) {
  if ( (unsigned int)(ch - 'a') < 26u )
    ch += 'A' - 'a';
  return ch;
}


int isprint (int ch) {
  ch&=0x7f;
  return (ch>=32 && ch<127);
}


int __ispunct_ascii ( int ch );
int __ispunct_ascii ( int ch ) 
{
    return isprint (ch)  &&  !isalnum (ch)  &&  !isspace (ch);
}
//int ispunct ( int ch ) __attribute__((weak,alias("__ispunct_ascii")));
int ispunct ( int ch ) __attribute__((alias("__ispunct_ascii")));


int iscntrl(int x) {
  unsigned char c=x&0xff;
  return (c<32) || (c>=127 && c<=160);
}


int __isxdigit_ascii ( int ch );
int __isxdigit_ascii ( int ch )
{
    return (unsigned int)( ch         - '0') < 10u  || 
           (unsigned int)((ch | 0x20) - 'a') <  6u;
}
//int isxdigit ( int ch ) __attribute__((weak,alias("__isxdigit_ascii")));
int isxdigit ( int ch ) __attribute__((alias("__isxdigit_ascii")));


int isgraph(int x) {
  unsigned char c=x&0xff;
  return (c>=33 && c<=126) || c>=161;
}












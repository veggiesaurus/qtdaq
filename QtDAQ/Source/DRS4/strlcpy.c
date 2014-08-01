/********************************************************************\

   Name:         strlcpy.c
   Created by:   Stefan Ritt

   Contents:     Contains strlcpy and strlcat which are versions of
                 strcpy and strcat, but which avoid buffer overflows

   $Id: strlcpy.c 16 2005-10-07 13:05:38Z ritt $

\********************************************************************/

#include <stdio.h>
#include <string.h>
#include "strlcpy.h"

/*
* Copy src to string dst of size siz.  At most siz-1 characters
* will be copied.  Always NUL terminates (unless size == 0).
* Returns strlen(src); if retval >= siz, truncation occurred.
*/
size_t strlcpy(char *dst, const char *src, size_t size)
{
   char *d = dst;
   const char *s = src;
   size_t n = size;

   /* Copy as many bytes as will fit */
   if (n != 0 && --n != 0) {
      do {
         if ((*d++ = *s++) == 0)
            break;
      } while (--n != 0);
   }

   /* Not enough room in dst, add NUL and traverse rest of src */
   if (n == 0) {
      if (size != 0)
         *d = '\0';             /* NUL-terminate dst */
      while (*s++);
   }

   return (s - src - 1);        /* count does not include NUL */
}

/*-------------------------------------------------------------------*/

/*
* Appends src to string dst of size siz (unlike strncat, siz is the
* full size of dst, not space left).  At most siz-1 characters
* will be copied.  Always NUL terminates (unless size <= strlen(dst)).
* Returns strlen(src) + MIN(size, strlen(initial dst)).
* If retval >= size, truncation occurred.
*/
size_t strlcat(char *dst, const char *src, size_t size)
{
   char *d = dst;
   const char *s = src;
   size_t n = size;
   size_t dlen;

   /* Find the end of dst and adjust bytes left but don't go past end */
   while (n-- != 0 && *d != '\0')
      d++;
   dlen = d - dst;
   n = size - dlen;

   if (n == 0)
      return (dlen + strlen(s));
   while (*s != '\0') {
      if (n != 1) {
         *d++ = *s;
         n--;
      }
      s++;
   }
   *d = '\0';

   return (dlen + (s - src));   /* count does not include NUL */
}

/*-------------------------------------------------------------------*/

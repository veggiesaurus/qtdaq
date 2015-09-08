/********************************************************************\

   Name:         strlcpy.h
   Created by:   Stefan Ritt

   Contents:     Header file for strlcpy.c

   $Id: strlcpy.h 62 2007-10-23 17:53:45Z sawada $

\********************************************************************/

#ifndef _STRLCPY_H_
#define _STRLCPY_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EXPRT
#if defined(EXPORT_DLL)
#define EXPRT __declspec(dllexport)
#else
#define EXPRT
#endif
#endif

size_t EXPRT strlcpy(char *dst, const char *src, size_t size);
size_t EXPRT strlcat(char *dst, const char *src, size_t size);

#ifdef __cplusplus
}
#endif

#endif /*_STRLCPY_H_ */

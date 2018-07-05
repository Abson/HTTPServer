#ifndef BASE_STRINGUTILS_H
#define BASE_STRINGUTILS_H

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

template <typename CTYPE>
size_t vsprintfn(CTYPE* buffer, size_t buflen, const CTYPE* format, va_list args) {
  int len = vsnprintf(buffer, buflen, format, args);
  if (len < 0 || static_cast<size_t>(len) >= buflen) {
    len = static_cast<size_t>(buflen - 1);
    buffer[len] = 0;
  }
  return len;
}

template <typename CTYPE>
size_t sprintfn(CTYPE* buffer, size_t buflen, const CTYPE* format, ...);
template <typename CTYPE>
size_t sprintfn(CTYPE* buffer, size_t buflen, const CTYPE* format, ...) {
  va_list args;
  va_start(args, format);
  size_t len = vsprintfn(buffer, buflen, format, args);
  va_end(args);
  return len;
}

#endif

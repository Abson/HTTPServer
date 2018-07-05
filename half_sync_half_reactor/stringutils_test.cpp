#include "stringutils.h"
#include <iostream>

const char* ok_string = "OK";

char write_buf[1024];

template <typename CTYPE>
bool AddResponse(const CTYPE* format, ...)  {
  va_list args;
  va_start(args, format);
  size_t len = vsprintfn(write_buf, sizeof(write_buf), format, args);
  va_end(args);
  return true;
}

int main(int argc, const char* argv[]) 
{
  AddResponse("HTTP/1.1 %s %d %s \r\r", "fuck", 200, ok_string);
  std::cout << write_buf << std::endl;
  return 0;
}

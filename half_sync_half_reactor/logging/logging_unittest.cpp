#include "logging.h"
#include "timezone.h"

#include <sys/types.h>
#include <unistd.h>

void* logInThread(void* arg)
{
  LOG_INFO << "logInThread";
  return nullptr;
}

int main(int argc, const char* argv[])
{
  getpid();

  pthread_t thread;
  pthread_create(&thread, nullptr, logInThread, nullptr);
  pthread_join(thread, nullptr);

  /* sleep(1); */
  /* base::TimeZone beijing(8*3600, "CST"); */
  /* base::Logger::setTimezone(beijing); */
  /* LOG_TRACE << "trace CST"; */
  /* LOG_DEBUG << "debug CST"; */
  /* LOG_INFO << "Hello CST"; */
  /* LOG_WARN << "World CST"; */
  /* LOG_ERROR << "Error CST"; */

  return 0;
}

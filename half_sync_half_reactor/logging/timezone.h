#ifndef BASE_TIMEZONE_H
#define BASE_TIMEZONE_H

#include <memory>

namespace base
{

class TimeZone
{
public:
  explicit TimeZone(const char* zonefile);
  TimeZone(int eastOfUtc, const char* tzname);
  TimeZone() {};

  bool valid() const
  {
    return static_cast<bool>(data_);
  }

  struct tm toLocalTime(time_t secondsSinceEpoch) const;
  time_t fromLocalTime(const struct tm&) const;

  /* gmtime(3) */
  static struct tm toUtcTime(time_t secondsSinceEpoch, bool yday = false);
  /* timegm(3) */
  static time_t fromUtcTime(const struct tm&);
  static time_t fromUtcTime(int year, int month, int day, 
      int hour, int minute, int seconds);

  struct Data;

private:
  std::shared_ptr<Data> data_;
};

}

#endif

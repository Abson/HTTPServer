#include "timezone.h"
#include "../commont/constructormagic.h"
#include "date.h"

#include <endian.h>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <string.h>
#include <time.h>

namespace base 
{
namespace detail
{

struct Transition
{
  time_t gmttime;
  time_t localtime;
  int localtime_idx;

  Transition(time_t t, time_t l, int local_idx) 
    : gmttime(t), localtime(l), localtime_idx(local_idx)
  {}
};

struct Comp
{
  bool compare_gmt;

  explicit Comp(bool gmt) 
    : compare_gmt(gmt)
  {}

  bool operator()(const Transition& lhs, const Transition& rhs) const
  {
    if (compare_gmt)
      return  lhs.gmttime < rhs.gmttime;
    else 
      return  lhs.localtime < rhs.localtime;
  }

  bool equal(const Transition& lhs, const Transition& rhs) const
  {
    if (compare_gmt)
      return  lhs.gmttime == rhs.gmttime;
    else 
      return  lhs.localtime == rhs.localtime;
  }
};

struct Localtime
{
  time_t gmt_offset;
  bool is_dst;
  int arrb_idx;

  Localtime(time_t offset, bool dst, int arrb) 
    : gmt_offset(offset), is_dst(dst), arrb_idx(arrb)
  {}
};

inline void FillHMS(unsigned seconds, struct tm* utc)
{
  utc->tm_sec = seconds % 60;
  unsigned minutes = seconds / 60;
  utc->tm_min = minutes % 60;
  utc->tm_hour = minutes / 60;
}

const int kSecondsPerDay = 24 * 60 * 60;

}
}


using namespace base;
struct TimeZone::Data 
{
  std::vector<detail::Transition> transitions;
  std::vector<detail::Localtime> localtimes;
  std::vector<std::string> names;
  std::string abbreviation;
};


namespace base
{
namespace detail
{

class File
{
public:
  explicit File(const char* file) 
    : fp_(::fopen(file, "rb"))
  {}

  ~File()
  {
    if (fp_)
    {
      ::fclose(fp_);
    }
  }

  bool valid() const { return fp_; }

  std::string readBytes(int n)
  {
    char buf[n];
    ssize_t nr = ::fread(buf, 1, n, fp_);
    if (nr != n)
      throw std::logic_error("no enough data");
    return std::string(buf, n);
  }

  int32_t readInt32()
  {
    int32_t x = 0;
    ssize_t nr = ::fread(&x, 1, sizeof(int32_t), fp_);
    if (nr != sizeof(int32_t))
      throw std::logic_error("bad int32_t data");
    return be32toh(x);
  }

  uint8_t readUInt8()
  {
    uint8_t x = 0;
    ssize_t nr = ::fread(&x, 1, sizeof(uint8_t), fp_);
    if (nr != sizeof(uint8_t))
      throw std::logic_error("bad uint8_t data");
    return x;
  }

private:
  FILE* fp_;

  BASE_DISALLOW_IMPLICIT_CONSTRUCTORS(File);
};

using namespace base;

bool readTimeZoneFile(const char* zonefile, struct TimeZone::Data* data)
{
  std::cout << __func__ << std::endl;

  File f(zonefile);
  if (f.valid())
  {
    try
    {
      std::string head = f.readBytes(4);
      if (head != "TZif")
        throw std::logic_error("bad head");
      std::string version = f.readBytes(1);
      f.readBytes(15);

      int32_t isgmtcnt = f.readInt32();
      int32_t isstdcnt = f.readInt32();
      int32_t leapcnt = f.readInt32();
      int32_t timecnt = f.readInt32();
      int32_t typecnt = f.readInt32();
      int32_t charcnt = f.readInt32();

      std::vector<int32_t> trans;
      std::vector<int> localtimes;
      trans.reserve(timecnt);
      for (int i = 0; i < timecnt; ++i)
      {
        trans.push_back(f.readInt32());
      }

      for (int i = 0; i < timecnt; ++i)
      {
        uint8_t local = f.readUInt8();
        localtimes.push_back(local);
      }

      for (int i = 0; i < typecnt; ++i)
      {
        int32_t gmtoff = f.readInt32();
        uint8_t isdst = f.readUInt8();
        uint8_t abbrind = f.readUInt8();

        data->localtimes.push_back(detail::Localtime(gmtoff, isdst, abbrind));
      }

      for (int i = 0; i < timecnt; ++i)
      {
        int local_idx = localtimes[i];
        time_t localtime = trans[i] + data->localtimes[local_idx].gmt_offset;
        data->transitions.push_back(Transition(trans[i], localtime, local_idx));
      }

      data->abbreviation = f.readBytes(charcnt);

      // leapcnt
      for (int i = 0; i < leapcnt; ++i)
      {
      }

      static_cast<void>(isstdcnt);
      static_cast<void>(isgmtcnt);
      static_cast<void>(version);
    }
    catch (std::logic_error& e)
    {
      fprintf(stderr, "%s\n", e.what());
    }
  }
  return true;
}

const Localtime* findLocalTime(const TimeZone::Data& data, Transition sentry, Comp comp)
{
  const Localtime* local = nullptr;

  if (data.transitions.empty() || comp(sentry, data.transitions.front())) 
  {
    local = &data.localtimes.front();
  }
  else
  {
    auto trans_i = std::lower_bound(data.transitions.begin(),
        data.transitions.end(), sentry, comp);

    if (trans_i != data.transitions.end())
    {
      if (!comp.equal(sentry, *trans_i))
      {
        assert(trans_i != data.transitions.begin());
        --trans_i;
      }
      local = &data.localtimes[trans_i->localtime_idx];
    }
    else 
    {
      local = &data.localtimes[data.transitions.back().localtime_idx];
    }
  }
  return local;
}

}
}

TimeZone::TimeZone(const char* zonefile) 
  : data_(new TimeZone::Data)
{
  if (!detail::readTimeZoneFile(zonefile, data_.get()))
  {
    data_.reset();
  } 
}

TimeZone::TimeZone(int eastOfUtc, const char* name) 
  : data_(new TimeZone::Data)
{
  data_->localtimes.push_back(detail::Localtime(eastOfUtc, false, 0));
  data_->abbreviation = name;
}

struct tm TimeZone::toLocalTime(time_t seconds) const
{
  std::cout << __func__ << std::endl;

  struct tm local_time;
  bzero(&local_time, sizeof(local_time));
  assert(data_ != nullptr);
  const Data& data(*data_);

  detail::Transition sentry(seconds, 0, 0);
  const detail::Localtime* local = detail::findLocalTime(data, sentry, detail::Comp(true));

  if (local)
  {
    time_t local_seconds = seconds + local->gmt_offset;
    ::gmtime_r(&local_seconds, &local_time);
    local_time.tm_isdst = local->is_dst;
    local_time.tm_gmtoff = local->gmt_offset;
    local_time.tm_zone = &data.abbreviation[local->arrb_idx];
  }

  local_time = *(::localtime(nullptr));
  return local_time;
}

time_t TimeZone::fromLocalTime(const struct tm& local_tm) const
{
  assert(data_ != nullptr);
  const Data& data(*data_);

  struct tm tmp = local_tm;
  time_t seconds = ::timegm(&tmp);
  detail::Transition sentry(0, seconds, 0);
  const detail::Localtime* local = detail::findLocalTime(data, sentry, detail::Comp(false));

  if (local_tm.tm_isdst)
  {
    struct tm try_tm = toLocalTime(seconds - local->gmt_offset);
    if (!try_tm.tm_isdst 
        && try_tm.tm_hour == local_tm.tm_hour 
        && try_tm.tm_min == local_tm.tm_min)
    {
      seconds -= 3600;
    }
  }
  return seconds - local->gmt_offset;
}

struct tm TimeZone::toUtcTime(time_t secondsSinceEpoch, bool yday)
{
  struct tm utc;
  bzero(&utc, sizeof(utc));
  utc.tm_zone = "GMT";
   
  int seconds = static_cast<int>(secondsSinceEpoch % detail::kSecondsPerDay);
  int days = static_cast<int>(secondsSinceEpoch / detail::kSecondsPerDay);
  if (seconds < 0)
  {
    seconds += detail::kSecondsPerDay;
    --days;
  }
  detail::FillHMS(seconds, &utc);

  /* Date */
  Date date(days + Date::kJulianDayOf1970_01_01);
  Date::YearMonthDay ymd = date.yearMonthDay();
  utc.tm_year = ymd.year - 1900;
  utc.tm_mon = ymd.month - 1;
  utc.tm_mday = ymd.day;
  utc.tm_wday = date.weekDay();

  if (yday)
  {
    Date start_of_year(ymd.year, 1, 1);
    utc.tm_yday = date.julianDayNumber() - start_of_year.julianDayNumber();
  }

  return utc;
}

time_t TimeZone::fromUtcTime(const struct tm& utc)
{
  return fromUtcTime(utc.tm_year + 1970, utc.tm_mon + 1, utc.tm_mday,
      utc.tm_hour, utc.tm_min, utc.tm_sec);
}

time_t TimeZone::fromUtcTime(int year, int month, int day, 
    int hour, int minute, int seconds)
{
  Date date(year, month, day);
  int seconds_in_day = hour * 3600 + minute * 60 + seconds;
  time_t days = date.julianDayNumber() - Date::kJulianDayOf1970_01_01;
  return  days * detail::kSecondsPerDay + seconds_in_day;
}


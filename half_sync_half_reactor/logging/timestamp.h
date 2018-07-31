#ifndef BASE_TIMESTAMP_H
#define BASE_TIMESTAMP_H

#include <iostream>

namespace base
{

class Timestamp 
{
public:
  Timestamp() 
    : microSecondsSinceEpoch_(0)
  {
  }

  explicit Timestamp(int64_t microSecondsSinceEpoch) 
    : microSecondsSinceEpoch_(microSecondsSinceEpoch)
  {
  }

  void swap(Timestamp& that)
  {
    std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
  }

  std::string toString() const;
  std::string toFormattedString(bool showMicroseconds = true) const;

  bool valid() const { return microSecondsSinceEpoch_ > 0; }

  int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
  time_t secondsSinceEpoch() const 
  {
    return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
  }

  static Timestamp now();
  static Timestamp Invalid()
  {
    return Timestamp();
  }

  static Timestamp fromUnixTime(time_t t)
  {
    return fromUnixTime(t, 0);
  }

  static Timestamp fromUnixTime(time_t t, int microseconds)
  {
    return Timestamp(static_cast<int64_t>(t) * kMicroSecondsPerSecond + microseconds);
  }

public:
  static constexpr int kMicroSecondsPerSecond = 1000 * 1000;

private:
  int64_t microSecondsSinceEpoch_;
};

inline bool operator<(Timestamp lhs, Timestamp rhs)
{
  return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs)
{
  return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

inline double TimeDifference(Timestamp high, Timestamp low) 
{
  int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
  return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}

inline Timestamp addTime(Timestamp timestamp, double seconds) 
{
  int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
  return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}

}

#endif

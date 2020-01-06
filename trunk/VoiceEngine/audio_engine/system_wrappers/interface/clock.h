
#ifndef VOIP_SYSTEM_WRAPPERS_INTERFACE_CLOCK_H_
#define VOIP_SYSTEM_WRAPPERS_INTERFACE_CLOCK_H_

#include "audio_engine/include/typedefs.h"

namespace VoIP {

// January 1970, in NTP seconds.
const uint32_t kNtpJan1970 = 2208988800UL;

// Magic NTP fractional unit.
const double kMagicNtpFractionalUnit = 4.294967296E+9;

// A clock interface that allows reading of absolute and relative timestamps.
class Clock {
 public:
  virtual ~Clock() {}

  // Return a timestamp in milliseconds relative to some arbitrary source; the
  // source is fixed for this clock.
  virtual int64_t TimeInMilliseconds() = 0;

  // Return a timestamp in microseconds relative to some arbitrary source; the
  // source is fixed for this clock.
  virtual int64_t TimeInMicroseconds() = 0;

  // Retrieve an NTP absolute timestamp in seconds and fractions of a second.
  virtual void CurrentNtp(uint32_t& seconds, uint32_t& fractions) = 0;

  // Retrieve an NTP absolute timestamp in milliseconds.
  virtual int64_t CurrentNtpInMilliseconds() = 0;

  // Converts an NTP timestamp to a millisecond timestamp.
  static int64_t NtpToMs(uint32_t seconds, uint32_t fractions);

  // Returns an instance of the real-time system clock implementation.
  static Clock* GetRealTimeClock();
};

class SimulatedClock : public Clock {
 public:
  explicit SimulatedClock(int64_t initial_time_us);

  virtual ~SimulatedClock() {}

  // Return a timestamp in milliseconds relative to some arbitrary source; the
  // source is fixed for this clock.
  virtual int64_t TimeInMilliseconds() OVERRIDE;

  // Return a timestamp in microseconds relative to some arbitrary source; the
  // source is fixed for this clock.
  virtual int64_t TimeInMicroseconds() OVERRIDE;

  // Retrieve an NTP absolute timestamp in milliseconds.
  virtual void CurrentNtp(uint32_t& seconds, uint32_t& fractions) OVERRIDE;

  // Converts an NTP timestamp to a millisecond timestamp.
  virtual int64_t CurrentNtpInMilliseconds() OVERRIDE;

  // Advance the simulated clock with a given number of milliseconds or
  // microseconds.
  void AdvanceTimeMilliseconds(int64_t milliseconds);
  void AdvanceTimeMicroseconds(int64_t microseconds);

 private:
  int64_t time_us_;
};

};  // namespace VoIP

#endif  // VOIP_SYSTEM_WRAPPERS_INTERFACE_CLOCK_H_

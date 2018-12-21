//
// System-specific implementation of the clock functions.
//
// Copyright (C) 2011 Nick Bruun <nick@bruun.co>
// Copyright (C) 2013 Vlad Lazarenko <vlad@lazarenko.me>
// Copyright (C) 2014 Nicolas Pauss <nicolas.pauss@gmail.com>
//
// Implementation notes:
//
// On Windows, QueryPerformanceCounter() is used. It gets
// real-time clock with up to nanosecond precision.
//
// On Apple (OS X, iOS), mach_absolute_time() is used. It gets
// CPU/bus dependent real-time clock with up to nanosecond precision.
//
// On Unix, gethrtime() is used with HP-UX and Solaris. Otherwise,
// clock_gettime() is used to access monotonic real-time clock
// with up to nanosecond precision. On kernels 2.6.28 and newer, the ticks
// are also raw and are not subject to NTP and/or adjtime(3) adjustments.
//
// Other POSIX compliant platforms resort to using gettimeofday(). It is
// subject to clock adjustments, does not allow for higher than microsecond
// resolution and is also declared obsolete by POSIX.1-2008.
//
// Note on C++11:
//
// Starting with C++11, we could use std::chrono. However, the details of
// what clock is really being used is implementation-specific. For example,
// Visual Studio 2012 defines "high_resolution_clock" as system clock with
// ~1 millisecond precision that is not acceptable for performance
// measurements. Therefore, we are much better off having full control of what
// mechanism we use to obtain the system clock.
//
// Note on durations: it is assumed that end times passed to the clock methods
// are all after the start time. Wrap-around of clocks is not tested, as
// nanosecond precision of unsigned 64-bit integers would require an uptime of
// almost 585 years for this to happen. Let's call ourselves safe on that one.
//
#ifndef __HAYAI_CLOCK
#define __HAYAI_CLOCK

#include "hayai_compatibility.hpp"


// POSIX
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#endif

// Win32
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

// Apple
#elif defined(__APPLE__) && defined(__MACH__)
#include <mach/mach_time.h>

// Unix
#elif defined(__unix__) || defined(__unix) || defined(unix)

// gethrtime
#   if (defined(__hpux) || defined(hpux)) || ((defined(__sun__) || defined(__sun) || defined(sun)) && (defined(__SVR4) || defined(__svr4__)))
#   include <sys/time.h>

// clock_gettime
#   elif defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)
#   include <time.h>

// gettimeofday
#   else
#   include <sys/time.h>

#   endif
#else
#error "Unable to define high resolution timer for an unknown OS."
#endif

#include <stdexcept>
#include <stdint.h>


namespace hayai
{
// Win32
#if defined(_WIN32)
    class Clock
    {
    public:
        /// Time point.

        /// Opaque representation of a point in time.
        typedef LARGE_INTEGER TimePoint;


        /// Get the current time as a time point.

        /// @returns the current time point.
        static TimePoint Now()
        {
            TimePoint result;
            QueryPerformanceCounter(&result);
            return result;
        }


        /// Get the duration between two time points.

        /// @param startTime Start time point.
        /// @param endTime End time point.
        /// @returns the number of nanoseconds elapsed between the two time
        /// points.
        static uint64_t Duration(const TimePoint& startTime,
                                 const TimePoint& endTime)
        {
            const static double performanceFrequencyNs =
                PerformanceFrequencyNs();

            return static_cast<uint64_t>(
                (endTime.QuadPart - startTime.QuadPart)
                * performanceFrequencyNs
            );
        }


        /// Clock implementation description.

        /// @returns a description of the clock implementation used.
        static const char* Description()
        {
            return "QueryPerformanceCounter";
        }
    private:
        static double PerformanceFrequencyNs()
        {
            TimePoint result;
            QueryPerformanceFrequency(&result);
            return 1e9 / static_cast<double>(result.QuadPart);
        }
    };

// Mach kernel.
#elif defined(__APPLE__) && defined(__MACH__)
    class Clock
    {
    public:
        /// Time point.

        /// Opaque representation of a point in time.
        typedef uint64_t TimePoint;


        /// Get the current time as a time point.

        /// @returns the current time point.
        static TimePoint Now() __hayai_noexcept
        {
            return mach_absolute_time();
        }


        /// Get the duration between two time points.

        /// @param startTime Start time point.
        /// @param endTime End time point.
        /// @returns the number of nanoseconds elapsed between the two time
        /// points.
        static uint64_t Duration(const TimePoint& startTime,
                                 const TimePoint& endTime) __hayai_noexcept
        {
            mach_timebase_info_data_t time_info;
            mach_timebase_info(&time_info);

            return (endTime - startTime) * time_info.numer / time_info.denom;
        }


        /// Clock implementation description.

        /// @returns a description of the clock implementation used.
        static const char* Description()
        {
            return "mach_absolute_time";
        }
    };

// Unix
#elif defined(__unix__) || defined(__unix) || defined(unix)

// gethrtime
#   if (defined(__hpux) || defined(hpux)) || ((defined(__sun__) || defined(__sun) || defined(sun)) && (defined(__SVR4) || defined(__svr4__)))
    class Clock
    {
    public:
        /// Time point.

        /// Opaque representation of a point in time.
        typedef hrtime_t TimePoint;


        /// Get the current time as a time point.

        /// @returns the current time point.
        static TimePoint Now() __hayai_noexcept
        {
            return gethrtime();
        }


        /// Get the duration between two time points.

        /// @param startTime Start time point.
        /// @param endTime End time point.
        /// @returns the number of nanoseconds elapsed between the two time
        /// points.
        static uint64_t Duration(const TimePoint& startTime,
                                 const TimePoint& endTime) __hayai_noexcept
        {
            return static_cast<uint64_t>(endTime - startTime);
        }


        /// Clock implementation description.

        /// @returns a description of the clock implementation used.
        static const char* Description()
        {
            return "gethrtime";
        }
    };


// clock_gettime
#   elif defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)
    class Clock
    {
    public:
        /// Time point.

        /// Opaque representation of a point in time.
        typedef struct timespec TimePoint;


        /// Get the current time as a time point.

        /// @returns the current time point.
        static TimePoint Now() __hayai_noexcept
        {
            TimePoint result;
#       if   defined(CLOCK_MONOTONIC_RAW)
            clock_gettime(CLOCK_MONOTONIC_RAW, &result);
#       elif defined(CLOCK_MONOTONIC)
            clock_gettime(CLOCK_MONOTONIC, &result);
#       elif defined(CLOCK_REALTIME)
            clock_gettime(CLOCK_REALTIME, &result);
#       else
            clock_gettime((clocId_t)-1, &result);
#       endif
            return result;
        }


        /// Get the duration between two time points.

        /// @param startTime Start time point.
        /// @param endTime End time point.
        /// @returns the number of nanoseconds elapsed between the two time
        /// points.
        static uint64_t Duration(const TimePoint& startTime,
                                 const TimePoint& endTime) __hayai_noexcept
        {
            TimePoint timeDiff;

            timeDiff.tv_sec = endTime.tv_sec - startTime.tv_sec;
            if (endTime.tv_nsec < startTime.tv_nsec)
            {
                timeDiff.tv_nsec = endTime.tv_nsec + 1000000000LL -
                    startTime.tv_nsec;
                timeDiff.tv_sec--;
            }
            else
                timeDiff.tv_nsec = endTime.tv_nsec - startTime.tv_nsec;

            return static_cast<uint64_t>(timeDiff.tv_sec * 1000000000LL +
                                         timeDiff.tv_nsec);
        }


        /// Clock implementation description.

        /// @returns a description of the clock implementation used.
        static const char* Description()
        {
#       if   defined(CLOCK_MONOTONIC_RAW)
            return "clock_gettime(CLOCK_MONOTONIC_RAW)";
#       elif defined(CLOCK_MONOTONIC)
            return "clock_gettime(CLOCK_MONOTONIC)";
#       elif defined(CLOCK_REALTIME)
            return "clock_gettime(CLOCK_REALTIME)";
#       else
            return "clock_gettime(-1)";
#       endif
        }
    };

// gettimeofday
#   else
    class Clock
    {
    public:
        /// Time point.

        /// Opaque representation of a point in time.
        typedef struct timeval TimePoint;


        /// Get the current time as a time point.

        /// @returns the current time point.
        static TimePoint Now() __hayai_noexcept
        {
            TimePoint result;
            gettimeofday(&result, NULL);
            return result;
        }


        /// Get the duration between two time points.

        /// @param startTime Start time point.
        /// @param endTime End time point.
        /// @returns the number of nanoseconds elapsed between the two time
        /// points.
        static uint64_t Duration(const TimePoint& startTime,
                                 const TimePoint& endTime) __hayai_noexcept
        {
            TimePoint timeDiff;

            timeDiff.tv_sec = endTime.tv_sec - startTime.tv_sec;
            if (endTime.tv_usec < startTime.tv_usec)
            {
                timeDiff.tv_usec = endTime.tv_usec + 1000000L -
                    startTime.tv_usec;
                timeDiff.tv_sec--;
            }
            else
                timeDiff.tv_usec = endTime.tv_usec - startTime.tv_usec;

            return static_cast<uint64_t>(timeDiff.tv_sec * 1000000000LL +
                                         timeDiff.tv_usec * 1000);
        }


        /// Clock implementation description.

        /// @returns a description of the clock implementation used.
        static const char* Description()
        {
            return "gettimeofday";
        }
    };
#   endif
#endif
}
#endif

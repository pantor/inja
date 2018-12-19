#ifndef __HAYAI_TESTRESULT
#define __HAYAI_TESTRESULT
#include <vector>
#include <stdexcept>
#include <limits>
#include <cmath>

#include "hayai_clock.hpp"


namespace hayai
{
    /// Test result descriptor.

    /// All durations are expressed in nanoseconds.
    struct TestResult
    {
    public:
        /// Initialize test result descriptor.

        /// @param runTimes Timing for the individual runs.
        /// @param iterations Number of iterations per run.
        TestResult(const std::vector<uint64_t>& runTimes,
                   std::size_t iterations)
            :   _runTimes(runTimes),
                _iterations(iterations),
                _timeTotal(0),
                _timeRunMin(std::numeric_limits<uint64_t>::max()),
                _timeRunMax(std::numeric_limits<uint64_t>::min()),
                _timeStdDev(0.0),
                _timeMedian(0.0),
                _timeQuartile1(0.0),
                _timeQuartile3(0.0)
        {
            // Summarize under the assumption of values being accessed more
            // than once.
            std::vector<uint64_t>::iterator runIt = _runTimes.begin();

            while (runIt != _runTimes.end())
            {
                const uint64_t run = *runIt;

                _timeTotal += run;
                if ((runIt == _runTimes.begin()) || (run > _timeRunMax))
                    _timeRunMax = run;
                if ((runIt == _runTimes.begin()) || (run < _timeRunMin))
                    _timeRunMin = run;

                ++runIt;
            }

            // Calculate standard deviation.
            const double mean = RunTimeAverage();
            double accu = 0.0;

            runIt = _runTimes.begin();

            while (runIt != _runTimes.end())
            {
                const uint64_t run = *runIt;
                const double diff = double(run) - mean;
                accu += (diff * diff);

                ++runIt;
            }

            _timeStdDev = std::sqrt(accu / (_runTimes.size() - 1));

            // Calculate quartiles.
            std::vector<uint64_t> sortedRunTimes(_runTimes);
            std::sort(sortedRunTimes.begin(), sortedRunTimes.end());

            const std::size_t sortedSize = sortedRunTimes.size();
            const std::size_t sortedSizeHalf = sortedSize / 2;

            if (sortedSize >= 2)
            {
                const std::size_t quartile = sortedSizeHalf / 2;

                if ((sortedSize % 2) == 0)
                {
                    _timeMedian =
                        (double(sortedRunTimes[sortedSizeHalf - 1]) +
                         double(sortedRunTimes[sortedSizeHalf])) / 2;

                    _timeQuartile1 =
                        double(sortedRunTimes[quartile]);
                    _timeQuartile3 =
                        double(sortedRunTimes[sortedSizeHalf + quartile]);
                }
                else
                {
                    _timeMedian = double(sortedRunTimes[sortedSizeHalf]);

                    _timeQuartile1 =
                        (double(sortedRunTimes[quartile - 1]) +
                         double(sortedRunTimes[quartile])) / 2;
                    _timeQuartile3 = (
                        double(
                            sortedRunTimes[sortedSizeHalf + (quartile - 1)]
                        ) +
                        double(
                            sortedRunTimes[sortedSizeHalf + quartile]
                        )
                    ) / 2;
                }
            }
            else if (sortedSize > 0)
            {
                _timeQuartile1 = double(sortedRunTimes[0]);
                _timeQuartile3 = _timeQuartile1;
            }
        }


        /// Total time.
        inline double TimeTotal() const
        {
            return double(_timeTotal);
        }


        /// Run times.
        inline const std::vector<uint64_t>& RunTimes() const
        {
            return _runTimes;
        }


        /// Average time per run.
        inline double RunTimeAverage() const
        {
            return double(_timeTotal) / double(_runTimes.size());
        }

        /// Standard deviation time per run.
        inline double RunTimeStdDev() const
        {
            return _timeStdDev;
        }

        /// Median (2nd Quartile) time per run.
        inline double RunTimeMedian() const
        {
            return _timeMedian;
        }

        /// 1st Quartile time per run.
        inline double RunTimeQuartile1() const
        {
            return _timeQuartile1;
        }

        /// 3rd Quartile time per run.
        inline double RunTimeQuartile3() const
        {
            return _timeQuartile3;
        }

        /// Maximum time per run.
        inline double RunTimeMaximum() const
        {
            return double(_timeRunMax);
        }


        /// Minimum time per run.
        inline double RunTimeMinimum() const
        {
            return double(_timeRunMin);
        }


        /// Average runs per second.
        inline double RunsPerSecondAverage() const
        {
            return 1000000000.0 / RunTimeAverage();
        }

        /// Median (2nd Quartile) runs per second.
        inline double RunsPerSecondMedian() const
        {
            return 1000000000.0 / RunTimeMedian();
        }

        /// 1st Quartile runs per second.
        inline double RunsPerSecondQuartile1() const
        {
            return 1000000000.0 / RunTimeQuartile1();
        }

        /// 3rd Quartile runs per second.
        inline double RunsPerSecondQuartile3() const
        {
            return 1000000000.0 / RunTimeQuartile3();
        }

        /// Maximum runs per second.
        inline double RunsPerSecondMaximum() const
        {
            return 1000000000.0 / _timeRunMin;
        }


        /// Minimum runs per second.
        inline double RunsPerSecondMinimum() const
        {
            return 1000000000.0 / _timeRunMax;
        }


        /// Average time per iteration.
        inline double IterationTimeAverage() const
        {
            return RunTimeAverage() / double(_iterations);
        }

        /// Standard deviation time per iteration.
        inline double IterationTimeStdDev() const
        {
            return RunTimeStdDev() / double(_iterations);
        }

        /// Median (2nd Quartile) time per iteration.
        inline double IterationTimeMedian() const
        {
            return RunTimeMedian() / double(_iterations);
        }

        /// 1st Quartile time per iteration.
        inline double IterationTimeQuartile1() const
        {
            return RunTimeQuartile1() / double(_iterations);
        }

        /// 3rd Quartile time per iteration.
        inline double IterationTimeQuartile3() const
        {
            return RunTimeQuartile3() / double(_iterations);
        }

        /// Minimum time per iteration.
        inline double IterationTimeMinimum() const
        {
            return _timeRunMin / double(_iterations);
        }


        /// Maximum time per iteration.
        inline double IterationTimeMaximum() const
        {
            return _timeRunMax / double(_iterations);
        }


        /// Average iterations per second.
        inline double IterationsPerSecondAverage() const
        {
            return 1000000000.0 / IterationTimeAverage();
        }

        /// Median (2nd Quartile) iterations per second.
        inline double IterationsPerSecondMedian() const
        {
            return 1000000000.0 / IterationTimeMedian();
        }

        /// 1st Quartile iterations per second.
        inline double IterationsPerSecondQuartile1() const
        {
            return 1000000000.0 / IterationTimeQuartile1();
        }

        /// 3rd Quartile iterations per second.
        inline double IterationsPerSecondQuartile3() const
        {
            return 1000000000.0 / IterationTimeQuartile3();
        }

        /// Minimum iterations per second.
        inline double IterationsPerSecondMinimum() const
        {
            return 1000000000.0 / IterationTimeMaximum();
        }


        /// Maximum iterations per second.
        inline double IterationsPerSecondMaximum() const
        {
            return 1000000000.0 / IterationTimeMinimum();
        }
    private:
        std::vector<uint64_t> _runTimes;
        std::size_t _iterations;
        uint64_t _timeTotal;
        uint64_t _timeRunMin;
        uint64_t _timeRunMax;
        double _timeStdDev;
        double _timeMedian;
        double _timeQuartile1;
        double _timeQuartile3;
    };
}
#endif

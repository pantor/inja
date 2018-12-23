#ifndef __HAYAI_CONSOLEOUTPUTTER
#define __HAYAI_CONSOLEOUTPUTTER
#include "hayai_outputter.hpp"
#include "hayai_console.hpp"


namespace hayai
{
    /// Console outputter.

    /// Prints the result to standard output.
    class ConsoleOutputter
        :   public Outputter
    {
    public:
        /// Initialize console outputter.

        /// @param stream Output stream. Must exist for the entire duration of
        /// the outputter's use.
        ConsoleOutputter(std::ostream& stream = std::cout)
            :   _stream(stream)
        {

        }


        virtual void Begin(const std::size_t& enabledCount,
                           const std::size_t& disabledCount)
        {
            _stream << std::fixed;
            _stream << Console::TextGreen << "[==========]"
                    << Console::TextDefault << " Running "
                    << enabledCount
                    << (enabledCount == 1 ? " benchmark." : " benchmarks");

            if (disabledCount)
                _stream << ", skipping "
                        << disabledCount
                        << (disabledCount == 1 ?
                            " benchmark." :
                            " benchmarks");
            else
                _stream << ".";

            _stream << std::endl;
        }


        virtual void End(const std::size_t& executedCount,
                         const std::size_t& disabledCount)
        {
            _stream << Console::TextGreen << "[==========]"
                    << Console::TextDefault << " Ran " << executedCount
                    << (executedCount == 1 ?
                        " benchmark." :
                        " benchmarks");

            if (disabledCount)
                _stream << ", skipped "
                        << disabledCount
                        << (disabledCount == 1 ?
                            " benchmark." :
                            " benchmarks");
            else
                _stream << ".";

            _stream << std::endl;
        }


        inline void BeginOrSkipTest(const std::string& fixtureName,
                                    const std::string& testName,
                                    const TestParametersDescriptor& parameters,
                                    const std::size_t& runsCount,
                                    const std::size_t& iterationsCount,
                                    const bool skip)
        {
            if (skip)
                _stream << Console::TextCyan << "[ DISABLED ]";
            else
                _stream << Console::TextGreen << "[ RUN      ]";

            _stream << Console::TextYellow << " ";
            WriteTestNameToStream(_stream, fixtureName, testName, parameters);
            _stream << Console::TextDefault
                    << " (" << runsCount
                    << (runsCount == 1 ? " run, " : " runs, ")
                    << iterationsCount
                    << (iterationsCount == 1 ?
                        " iteration per run)" :
                        " iterations per run)")
                    << std::endl;
        }


        virtual void BeginTest(const std::string& fixtureName,
                               const std::string& testName,
                               const TestParametersDescriptor& parameters,
                               const std::size_t& runsCount,
                               const std::size_t& iterationsCount)
        {
            BeginOrSkipTest(fixtureName,
                            testName,
                            parameters,
                            runsCount,
                            iterationsCount,
                            false);
        }


        virtual void SkipDisabledTest(
            const std::string& fixtureName,
            const std::string& testName,
            const TestParametersDescriptor& parameters,
            const std::size_t& runsCount,
            const std::size_t& iterationsCount
        )
        {
            BeginOrSkipTest(fixtureName,
                            testName,
                            parameters,
                            runsCount,
                            iterationsCount,
                            true);
        }


        virtual void EndTest(const std::string& fixtureName,
                             const std::string& testName,
                             const TestParametersDescriptor& parameters,
                             const TestResult& result)
        {
#define PAD(x) _stream << std::setw(34) << x << std::endl;
#define PAD_DEVIATION(description,                                      \
                      deviated,                                         \
                      average,                                          \
                      unit)                                             \
            {                                                           \
                double _d_ =                                            \
                    double(deviated) - double(average);                 \
                                                                        \
                PAD(description <<                                      \
                    deviated << " " << unit << " (" <<                  \
                    (deviated < average ?                               \
                     Console::TextRed :                                 \
                     Console::TextGreen) <<                             \
                    (deviated > average ? "+" : "") <<                  \
                    _d_ << " " << unit << " / " <<                      \
                    (deviated > average ? "+" : "") <<                  \
                    (_d_ * 100.0 / average) << " %" <<                  \
                    Console::TextDefault << ")");                       \
            }
#define PAD_DEVIATION_INVERSE(description,                              \
                              deviated,                                 \
                              average,                                  \
                              unit)                                     \
            {                                                           \
                double _d_ =                                            \
                    double(deviated) - double(average);                 \
                                                                        \
                PAD(description <<                                      \
                    deviated << " " << unit << " (" <<                  \
                    (deviated > average ?                               \
                     Console::TextRed :                                 \
                     Console::TextGreen) <<                             \
                    (deviated > average ? "+" : "") <<                  \
                    _d_ << " " << unit << " / " <<                      \
                    (deviated > average ? "+" : "") <<                  \
                    (_d_ * 100.0 / average) << " %" <<                  \
                    Console::TextDefault << ")");                       \
            }

            _stream << Console::TextGreen << "[     DONE ]"
                    << Console::TextYellow << " ";
            WriteTestNameToStream(_stream, fixtureName, testName, parameters);
            _stream << Console::TextDefault << " ("
                    << std::setprecision(6)
                    << (result.TimeTotal() / 1000000.0) << " ms)"
                    << std::endl;

            _stream << Console::TextBlue << "[   RUNS   ] "
                    << Console::TextDefault
                    << "       Average time: "
                    << std::setprecision(3)
                    << result.RunTimeAverage() / 1000.0 << " us "
                    << "(" << Console::TextBlue << "~"
                    << result.RunTimeStdDev() / 1000.0 << " us"
                    << Console::TextDefault << ")"
                    << std::endl;

            PAD_DEVIATION_INVERSE("Fastest time: ",
                                  (result.RunTimeMinimum() / 1000.0),
                                  (result.RunTimeAverage() / 1000.0),
                                  "us");
            PAD_DEVIATION_INVERSE("Slowest time: ",
                                  (result.RunTimeMaximum() / 1000.0),
                                  (result.RunTimeAverage() / 1000.0),
                                  "us");
            PAD("Median time: " <<
                result.RunTimeMedian() / 1000.0 << " us (" <<
                Console::TextCyan << "1st quartile: " <<
                result.RunTimeQuartile1() / 1000.0 << " us | 3rd quartile: " <<
                result.RunTimeQuartile3() / 1000.0 << " us" <<
                Console::TextDefault << ")");

            _stream << std::setprecision(5);

            PAD("");
            PAD("Average performance: " <<
                result.RunsPerSecondAverage() << " runs/s");
            PAD_DEVIATION("Best performance: ",
                          result.RunsPerSecondMaximum(),
                          result.RunsPerSecondAverage(),
                          "runs/s");
            PAD_DEVIATION("Worst performance: ",
                          result.RunsPerSecondMinimum(),
                          result.RunsPerSecondAverage(),
                          "runs/s");
            PAD("Median performance: " <<
                result.RunsPerSecondMedian() << " runs/s (" <<
                Console::TextCyan << "1st quartile: " <<
                result.RunsPerSecondQuartile1() << " | 3rd quartile: " <<
                result.RunsPerSecondQuartile3() <<
                Console::TextDefault << ")");

            PAD("");
            _stream << Console::TextBlue << "[ITERATIONS] "
                    << Console::TextDefault
                    << std::setprecision(3)
                    << "       Average time: "
                    << result.IterationTimeAverage() / 1000.0 << " us "
                    << "(" << Console::TextBlue << "~"
                    << result.IterationTimeStdDev() / 1000.0 << " us"
                    << Console::TextDefault << ")"
                    << std::endl;

            PAD_DEVIATION_INVERSE("Fastest time: ",
                                  (result.IterationTimeMinimum() / 1000.0),
                                  (result.IterationTimeAverage() / 1000.0),
                                  "us");
            PAD_DEVIATION_INVERSE("Slowest time: ",
                                  (result.IterationTimeMaximum() / 1000.0),
                                  (result.IterationTimeAverage() / 1000.0),
                                  "us");
            PAD("Median time: " <<
                result.IterationTimeMedian() / 1000.0 << " us (" <<
                Console::TextCyan << "1st quartile: " <<
                result.IterationTimeQuartile1() / 1000.0 <<
                " us | 3rd quartile: " <<
                result.IterationTimeQuartile3() / 1000.0 << " us" <<
                Console::TextDefault << ")");

            _stream << std::setprecision(5);

            PAD("");
            PAD("Average performance: " <<
                result.IterationsPerSecondAverage() <<
                " iterations/s");
            PAD_DEVIATION("Best performance: ",
                          (result.IterationsPerSecondMaximum()),
                          (result.IterationsPerSecondAverage()),
                          "iterations/s");
            PAD_DEVIATION("Worst performance: ",
                          (result.IterationsPerSecondMinimum()),
                          (result.IterationsPerSecondAverage()),
                          "iterations/s");
            PAD("Median performance: " <<
                result.IterationsPerSecondMedian() << " iterations/s (" <<
                Console::TextCyan << "1st quartile: " <<
                result.IterationsPerSecondQuartile1() <<
                " | 3rd quartile: " <<
                result.IterationsPerSecondQuartile3() <<
                Console::TextDefault << ")");

#undef PAD_DEVIATION_INVERSE
#undef PAD_DEVIATION
#undef PAD
        }


        std::ostream& _stream;
    };
}
#endif

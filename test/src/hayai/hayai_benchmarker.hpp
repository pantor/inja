#ifndef __HAYAI_BENCHMARKER
#define __HAYAI_BENCHMARKER
#include <algorithm>
#include <vector>
#include <limits>
#include <iomanip>
#if __cplusplus > 201100L
#include <random>
#endif
#include <string>
#include <cstring>

#include "hayai_test_factory.hpp"
#include "hayai_test_descriptor.hpp"
#include "hayai_test_result.hpp"
#include "hayai_console_outputter.hpp"


namespace hayai
{
    /// Benchmarking execution controller singleton.
    class Benchmarker
    {
    public:
        /// Get the singleton instance of @ref Benchmarker.

        /// @returns a reference to the singleton instance of the
        /// benchmarker execution controller.
        static Benchmarker& Instance()
        {
            static Benchmarker singleton;
            return singleton;
        }


        /// Register a test with the benchmarker instance.

        /// @param fixtureName Name of the fixture.
        /// @param testName Name of the test.
        /// @param runs Number of runs for the test.
        /// @param iterations Number of iterations per run.
        /// @param testFactory Test factory implementation for the test.
        /// @returns a pointer to a @ref TestDescriptor instance
        /// representing the given test.
        static TestDescriptor* RegisterTest(
            const char* fixtureName,
            const char* testName,
            std::size_t runs,
            std::size_t iterations,
            TestFactory* testFactory,
            TestParametersDescriptor parameters
        )
        {
            // Determine if the test has been disabled.
            static const char* disabledPrefix = "DISABLED_";
            bool isDisabled = ((::strlen(testName) >= 9) &&
                               (!::memcmp(testName, disabledPrefix, 9)));

            if (isDisabled)
                testName += 9;

            // Add the descriptor.
            TestDescriptor* descriptor = new TestDescriptor(fixtureName,
                                                            testName,
                                                            runs,
                                                            iterations,
                                                            testFactory,
                                                            parameters,
                                                            isDisabled);

            Instance()._tests.push_back(descriptor);

            return descriptor;
        }


        /// Add an outputter.

        /// @param outputter Outputter. The caller must ensure that the
        /// outputter remains in existence for the entire benchmark run.
        static void AddOutputter(Outputter& outputter)
        {
            Instance()._outputters.push_back(&outputter);
        }


        /// Apply a pattern filter to the tests.

        /// --gtest_filter-compatible pattern:
        ///
        /// https://code.google.com/p/googletest/wiki/AdvancedGuide
        ///
        /// @param pattern Filter pattern compatible with gtest.
        static void ApplyPatternFilter(const char* pattern)
        {
            Benchmarker& instance = Instance();

            // Split the filter at '-' if it exists.
            const char* const dash = strchr(pattern, '-');

            std::string positive;
            std::string negative;

            if (dash == NULL)
                positive = pattern;
            else
            {
                positive = std::string(pattern, dash);
                negative = std::string(dash + 1);
                if (positive.empty())
                    positive = "*";
            }

            // Iterate across all tests and test them against the patterns.
            std::size_t index = 0;
            while (index < instance._tests.size())
            {
                TestDescriptor* desc = instance._tests[index];

                if ((!FilterMatchesString(positive.c_str(),
                                          desc->CanonicalName)) ||
                    (FilterMatchesString(negative.c_str(),
                                         desc->CanonicalName)))
                {
                    instance._tests.erase(
                        instance._tests.begin() +
                        std::vector<TestDescriptor*>::difference_type(index)
                    );
                    delete desc;
                }
                else
                    ++index;
            }
        }


        /// Run all benchmarking tests.
        static void RunAllTests()
        {
            ConsoleOutputter defaultOutputter;
            std::vector<Outputter*> defaultOutputters;
            defaultOutputters.push_back(&defaultOutputter);

            Benchmarker& instance = Instance();
            std::vector<Outputter*>& outputters =
                (instance._outputters.empty() ?
                 defaultOutputters :
                 instance._outputters);

            // Get the tests for execution.
            std::vector<TestDescriptor*> tests = instance.GetTests();

            const std::size_t totalCount = tests.size();
            std::size_t disabledCount = 0;

            std::vector<TestDescriptor*>::const_iterator testsIt =
                tests.begin();

            while (testsIt != tests.end())
            {
                if ((*testsIt)->IsDisabled)
                    ++disabledCount;
                ++testsIt;
            }

            const std::size_t enabledCount = totalCount - disabledCount;

            // Calibrate the tests.
            const CalibrationModel calibrationModel = GetCalibrationModel();

            // Begin output.
            for (std::size_t outputterIndex = 0;
                 outputterIndex < outputters.size();
                 outputterIndex++)
                outputters[outputterIndex]->Begin(enabledCount, disabledCount);

            // Run through all the tests in ascending order.
            std::size_t index = 0;

            while (index < tests.size())
            {
                // Get the test descriptor.
                TestDescriptor* descriptor = tests[index++];

                // Check if test matches include filters
                if (instance._include.size() > 0)
                {
                    bool included = false;
                    std::string name =
                        descriptor->FixtureName + "." +
                        descriptor->TestName;

                    for (std::size_t i = 0; i < instance._include.size(); i++)
                    {
                        if (name.find(instance._include[i]) !=
                            std::string::npos)
                        {
                            included = true;
                            break;
                        }
                    }

                    if (!included)
                        continue;
                }

                // Check if test is not disabled.
                if (descriptor->IsDisabled)
                {
                    for (std::size_t outputterIndex = 0;
                         outputterIndex < outputters.size();
                         outputterIndex++)
                        outputters[outputterIndex]->SkipDisabledTest(
                            descriptor->FixtureName,
                            descriptor->TestName,
                            descriptor->Parameters,
                            descriptor->Runs,
                            descriptor->Iterations
                        );

                    continue;
                }

                // Describe the beginning of the run.
                for (std::size_t outputterIndex = 0;
                     outputterIndex < outputters.size();
                     outputterIndex++)
                    outputters[outputterIndex]->BeginTest(
                        descriptor->FixtureName,
                        descriptor->TestName,
                        descriptor->Parameters,
                        descriptor->Runs,
                        descriptor->Iterations
                    );

                // Execute each individual run.
                std::vector<uint64_t> runTimes(descriptor->Runs);
                uint64_t overheadCalibration =
                    calibrationModel.GetCalibration(descriptor->Iterations);

                std::size_t run = 0;
                while (run < descriptor->Runs)
                {
                    // Construct a test instance.
                    Test* test = descriptor->Factory->CreateTest();

                    // Run the test.
                    uint64_t time = test->Run(descriptor->Iterations);

                    // Store the test time.
                    runTimes[run] = (time > overheadCalibration ?
                                     time - overheadCalibration :
                                     0);

                    // Dispose of the test instance.
                    delete test;

                    ++run;
                }

                // Calculate the test result.
                TestResult testResult(runTimes, descriptor->Iterations);

                // Describe the end of the run.
                for (std::size_t outputterIndex = 0;
                     outputterIndex < outputters.size();
                     outputterIndex++)
                    outputters[outputterIndex]->EndTest(
                        descriptor->FixtureName,
                        descriptor->TestName,
                        descriptor->Parameters,
                        testResult
                    );

            }

            // End output.
            for (std::size_t outputterIndex = 0;
                 outputterIndex < outputters.size();
                 outputterIndex++)
                outputters[outputterIndex]->End(enabledCount,
                                                disabledCount);
        }


        /// List tests.
        static std::vector<const TestDescriptor*> ListTests()
        {
            std::vector<const TestDescriptor*> tests;
            Benchmarker& instance = Instance();

            std::size_t index = 0;
            while (index < instance._tests.size())
                tests.push_back(instance._tests[index++]);

            return tests;
        }


        /// Shuffle tests.

        /// Randomly shuffles the order of tests.
        static void ShuffleTests()
        {
            Benchmarker& instance = Instance();
#if __cplusplus > 201100L
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(instance._tests.begin(),
                         instance._tests.end(),
                         g);
#else
            std::random_shuffle(instance._tests.begin(),
                                instance._tests.end());
#endif
        }
    private:
        /// Calibration model.

        /// Describes a linear calibration model for test runs.
        struct CalibrationModel
        {
        public:
            CalibrationModel(std::size_t scale,
                             uint64_t slope,
                             uint64_t yIntercept)
                :   Scale(scale),
                    Slope(slope),
                    YIntercept(yIntercept)
            {

            }

            
            /// Scale.

            /// Number of iterations per slope unit.
            const std::size_t Scale;


            /// Slope.
            const uint64_t Slope;


            /// Y-intercept;
            const uint64_t YIntercept;


            /// Get calibration value for a run.
            int64_t GetCalibration(std::size_t iterations) const
            {
                return YIntercept + (iterations * Slope) / Scale;
            }
        };

        
        /// Private constructor.
        Benchmarker()
        {

        }


        /// Private destructor.
        ~Benchmarker()
        {
            // Release all test descriptors.
            std::size_t index = _tests.size();
            while (index--)
                delete _tests[index];
        }


        /// Get the tests to be executed.
        std::vector<TestDescriptor*> GetTests() const
        {
            std::vector<TestDescriptor*> tests;

            std::size_t index = 0;
            while (index < _tests.size())
                tests.push_back(_tests[index++]);

            return tests;
        }


        /// Test if a filter matches a string.

        /// Adapted from gtest. All rights reserved by original authors.
        static bool FilterMatchesString(const char* filter,
                                        const std::string& str)
        {
            const char *patternStart = filter;

            while (true)
            {
                if (PatternMatchesString(patternStart, str.c_str()))
                    return true;

                // Finds the next pattern in the filter.
                patternStart = strchr(patternStart, ':');

                // Returns if no more pattern can be found.
                if (!patternStart)
                    return false;

                // Skips the pattern separater (the ':' character).
                patternStart++;
            }
        }


        /// Test if pattern matches a string.

        /// Adapted from gtest. All rights reserved by original authors.
        static bool PatternMatchesString(const char* pattern, const char *str)
        {
            switch (*pattern)
            {
            case '\0':
            case ':':
                return (*str == '\0');
            case '?':  // Matches any single character.
                return ((*str != '\0') &&
                        (PatternMatchesString(pattern + 1, str + 1)));
            case '*':  // Matches any string (possibly empty) of characters.
                return (((*str != '\0') &&
                         (PatternMatchesString(pattern, str + 1))) ||
                        (PatternMatchesString(pattern + 1, str)));
            default:
                return ((*pattern == *str) &&
                        (PatternMatchesString(pattern + 1, str + 1)));
            }
        }


        /// Get calibration model.

        /// Returns an average linear calibration model.
        static CalibrationModel GetCalibrationModel()
        {
            // We perform a number of runs of varying iterations with an empty
            // test body. The assumption here is, that the time taken for the
            // test run is linear with regards to the number of iterations, ie.
            // some constant overhead with a per-iteration overhead. This
            // hypothesis has been manually validated by linear regression over
            // sample data.
            //
            // In order to avoid losing too much precision, we are going to
            // calibrate in terms of the overhead of some x n iterations,
            // where n must be a sufficiently large number to produce some
            // significant runtime. On a high-end 2012 Retina MacBook Pro with
            // -O3 on clang-602.0.53 (LLVM 6.1.0) n = 1,000,000 produces
            // run times of ~1.9 ms, which should be sufficiently precise.
            //
            // However, as the constant overhead is mostly related to
            // retrieving the system clock, which under the same conditions
            // clocks in at around 17 ms, we run the risk of winding up with
            // a negative y-intercept if we do not fix the y-intercept. This
            // intercept is therefore fixed by a large number of runs of 0
            // iterations.
            ::hayai::Test* test = new Test();

#define HAYAI_CALIBRATION_INTERESECT_RUNS 10000

#define HAYAI_CALIBRATION_RUNS 10
#define HAYAI_CALIBRATION_SCALE 1000000
#define HAYAI_CALIBRATION_PPR 6

            // Determine the intercept.
            uint64_t
                interceptSum = 0,
                interceptMin = std::numeric_limits<uint64_t>::min(),
                interceptMax = 0;

            for (std::size_t run = 0;
                 run < HAYAI_CALIBRATION_INTERESECT_RUNS;
                 ++run)
            {
                uint64_t intercept = test->Run(0);
                interceptSum += intercept;
                if (intercept < interceptMin)
                    interceptMin = intercept;
                if (intercept > interceptMax)
                    interceptMax = intercept;
            }

            uint64_t interceptAvg =
                interceptSum / HAYAI_CALIBRATION_INTERESECT_RUNS;

            // Produce a series of sample points.
            std::vector<uint64_t> x(HAYAI_CALIBRATION_RUNS *
                                    HAYAI_CALIBRATION_PPR);
            std::vector<uint64_t> t(HAYAI_CALIBRATION_RUNS *
                                    HAYAI_CALIBRATION_PPR);

            std::size_t point = 0;

            for (std::size_t run = 0; run < HAYAI_CALIBRATION_RUNS; ++run)
            {
#define HAYAI_CALIBRATION_POINT(_x)                                     \
                x[point] = _x;                                          \
                t[point++] =                                            \
                    test->Run(_x * std::size_t(HAYAI_CALIBRATION_SCALE))

                HAYAI_CALIBRATION_POINT(1);
                HAYAI_CALIBRATION_POINT(2);
                HAYAI_CALIBRATION_POINT(5);
                HAYAI_CALIBRATION_POINT(10);
                HAYAI_CALIBRATION_POINT(15);
                HAYAI_CALIBRATION_POINT(20);

#undef HAYAI_CALIBRATION_POINT
            }

            // As we have a fixed y-intercept, b, the optimal slope for a line
            // fitting the sample points will be
            // $\frac {\sum_{i=1}^{n} x_n \cdot (y_n - b)}
            //  {\sum_{i=1}^{n} {x_n}^2}$.
            uint64_t
                sumProducts = 0,
                sumXSquared = 0;

            std::size_t p = x.size();
            while (p--)
            {
                sumXSquared += x[p] * x[p];
                sumProducts += x[p] * (t[p] - interceptAvg);
            }

            uint64_t slope = sumProducts / sumXSquared;

            delete test;

            return CalibrationModel(HAYAI_CALIBRATION_SCALE,
                                    slope,
                                    interceptAvg);

#undef HAYAI_CALIBRATION_INTERESECT_RUNS

#undef HAYAI_CALIBRATION_RUNS
#undef HAYAI_CALIBRATION_SCALE
#undef HAYAI_CALIBRATION_PPR
        }


        std::vector<Outputter*> _outputters; ///< Registered outputters.
        std::vector<TestDescriptor*> _tests; ///< Registered tests.
        std::vector<std::string> _include; ///< Test filters.
    };
}
#endif

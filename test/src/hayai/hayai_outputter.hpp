#ifndef __HAYAI_OUTPUTTER
#define __HAYAI_OUTPUTTER
#include <iostream>
#include <cstddef>

#include "hayai_test_result.hpp"


namespace hayai
{
    /// Outputter.

    /// Abstract base class for outputters.
    class Outputter
    {
    public:
        /// Begin benchmarking.

        /// The total number of benchmarks registred is the sum of the two
        /// counts passed to the outputter.
        ///
        /// @param enabledCount Number of benchmarks to be executed.
        /// @param disabledCount Number of disabled benchmarks to be skipped.
        virtual void Begin(const std::size_t& enabledCount,
                           const std::size_t& disabledCount) = 0;


        /// End benchmarking.

        /// @param executedCount Number of benchmarks that have been executed.
        /// @param disabledCount Number of benchmarks that have been skipped
        /// because they are disabled.
        virtual void End(const std::size_t& executedCount,
                         const std::size_t& disabledCount) = 0;


        /// Begin benchmark test run.

        /// @param fixtureName Fixture name.
        /// @param testName Test name.
        /// @param parameters Test parameter description.
        /// @param runsCount Number of runs to be executed.
        /// @param iterationsCount Number of iterations per run.
        virtual void BeginTest(const std::string& fixtureName,
                               const std::string& testName,
                               const TestParametersDescriptor& parameters,
                               const std::size_t& runsCount,
                               const std::size_t& iterationsCount) = 0;


        /// End benchmark test run.

        /// @param fixtureName Fixture name.
        /// @param testName Test name.
        /// @param parameters Test parameter description.
        /// @param result Test result.
        virtual void EndTest(const std::string& fixtureName,
                             const std::string& testName,
                             const TestParametersDescriptor& parameters,
                             const TestResult& result) = 0;


        /// Skip disabled benchmark test run.

        /// @param fixtureName Fixture name.
        /// @param testName Test name.
        /// @param parameters Test parameter description.
        /// @param runsCount Number of runs to be executed.
        /// @param iterationsCount Number of iterations per run.
        virtual void SkipDisabledTest(const std::string& fixtureName,
                                      const std::string& testName,
                                      const TestParametersDescriptor&
                                          parameters,
                                      const std::size_t& runsCount,
                                      const std::size_t& iterationsCount) = 0;


        virtual ~Outputter()
        {

        }
    protected:
        /// Write a nicely formatted test name to a stream.
        static void WriteTestNameToStream(std::ostream& stream,
                                          const std::string& fixtureName,
                                          const std::string& testName,
                                          const TestParametersDescriptor&
                                              parameters)
        {
            stream << fixtureName << "." << testName;

            const std::vector<TestParameterDescriptor>& descs =
                parameters.Parameters();

            if (descs.empty())
                return;

            stream << "(";

            for (std::size_t i = 0; i < descs.size(); ++i)
            {
                if (i)
                    stream << ", ";

                const TestParameterDescriptor& desc = descs[i];
                stream << desc.Declaration << " = " << desc.Value;
            }

            stream << ")";
        }
    };
}
#endif

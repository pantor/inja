#ifndef __HAYAI_JUNITXMLOUTPUTTER
#define __HAYAI_JUNITXMLOUTPUTTER
#include <iomanip>
#include <ostream>
#include <vector>
#include <sstream>
#include <map>

#include "hayai_outputter.hpp"


namespace hayai
{
    /// JUnit-compatible XML outputter.
    class JUnitXmlOutputter
        :   public Outputter
    {
    private:
        /// Test case.
        class TestCase
        {
        public:
            TestCase(const std::string& fixtureName,
                     const std::string& testName,
                     const TestParametersDescriptor& parameters,
                     const TestResult* result)
            {
                // Derive a pretty name.
                std::stringstream nameStream;
                WriteTestNameToStream(nameStream,
                                      fixtureName,
                                      testName,
                                      parameters);
                Name = nameStream.str();

                // Derive the result.
                Skipped = !result;

                if (result)
                {
                    std::stringstream timeStream;
                    timeStream << std::fixed
                               << std::setprecision(9)
                               << (result->IterationTimeAverage() / 1e9);
                    Time = timeStream.str();
                }
            }


            std::string Name;
            std::string Time;
            bool Skipped;
        };


        /// Test suite map.
        typedef std::map<std::string, std::vector<TestCase> > TestSuiteMap;
    public:
        /// Initialize outputter.

        /// @param stream Output stream. Must exist for the entire duration of
        /// the outputter's use.
        JUnitXmlOutputter(std::ostream& stream)
            :   _stream(stream)
        {

        }


        virtual void Begin(const std::size_t& enabledCount,
                           const std::size_t& disabledCount)
        {
            (void)enabledCount;
            (void)disabledCount;
        }


        virtual void End(const std::size_t& executedCount,
                         const std::size_t& disabledCount)
        {
            (void)executedCount;
            (void)disabledCount;

            // Write the header.
            _stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                    << std::endl
                    << "<testsuites>" << std::endl;

            // Write out each test suite (fixture.)
            for (TestSuiteMap::iterator testSuiteIt = _testSuites.begin();
                 testSuiteIt != _testSuites.end();
                 ++testSuiteIt)
            {
                _stream << "    <testsuite name=\"" << testSuiteIt->first
                        << "\" tests=\"" << testSuiteIt->second.size() << "\">"
                        << std::endl;

                // Write out each test case.
                for (std::vector<TestCase>::iterator testCaseIt =
                         testSuiteIt->second.begin();
                     testCaseIt != testSuiteIt->second.end();
                     ++testCaseIt)
                {
                    _stream << "        <testcase name=\"";
                    WriteEscapedString(testCaseIt->Name);
                    _stream << "\"";

                    if (!testCaseIt->Skipped)
                        _stream << " time=\"" << testCaseIt->Time << "\" />"
                                << std::endl;
                    else
                    {
                        _stream << ">" << std::endl
                                << "            <skipped />" << std::endl
                                << "        </testcase>" << std::endl;
                    }
                }
                
                _stream << "    </testsuite>" << std::endl;
            }

            _stream << "</testsuites>" << std::endl;
        }


        virtual void BeginTest(const std::string& fixtureName,
                               const std::string& testName,
                               const TestParametersDescriptor& parameters,
                               const std::size_t& runsCount,
                               const std::size_t& iterationsCount)
        {
            (void)fixtureName;
            (void)testName;
            (void)parameters;
            (void)runsCount;
            (void)iterationsCount;
        }


        virtual void SkipDisabledTest(const std::string& fixtureName,
                                      const std::string& testName,
                                      const TestParametersDescriptor& parameters,
                                      const std::size_t& runsCount,
                                      const std::size_t& iterationsCount)
        {
            (void)fixtureName;
            (void)testName;
            (void)parameters;
            (void)runsCount;
            (void)iterationsCount;
        }


        virtual void EndTest(const std::string& fixtureName,
                             const std::string& testName,
                             const TestParametersDescriptor& parameters,
                             const TestResult& result)
        {
            (void)fixtureName;
            (void)testName;
            (void)parameters;

            TestSuiteMap::iterator fixtureIt =
                _testSuites.find(fixtureName);
            if (fixtureIt == _testSuites.end())
            {
                _testSuites[fixtureName] = std::vector<TestCase>();
                fixtureIt = _testSuites.find(fixtureName);
            }

            std::vector<TestCase>& testCases = fixtureIt->second;

            testCases.push_back(TestCase(fixtureName,
                                         testName,
                                         parameters,
                                         &result));

            /*
            _stream <<
                JSON_VALUE_SEPARATOR

                JSON_STRING_BEGIN "runs" JSON_STRING_END
                JSON_NAME_SEPARATOR
                JSON_ARRAY_BEGIN;

            const std::vector<uint64_t>& runTimes = result.RunTimes();

            for (std::vector<uint64_t>::const_iterator it = runTimes.begin();
                 it != runTimes.end();
                 ++it)
            {
                if (it != runTimes.begin())
                    _stream << JSON_VALUE_SEPARATOR;

                _stream << JSON_OBJECT_BEGIN

                           JSON_STRING_BEGIN "duration" JSON_STRING_END
                           JSON_NAME_SEPARATOR
                        << std::fixed
                        << std::setprecision(6)
                        << (double(*it) / 1000000.0)
                        << JSON_OBJECT_END;
            }

            _stream <<
                JSON_ARRAY_END;

            EndTestObject();
            */
        }
    private:
        /// Write an escaped string.

        /// The escaping is currently very rudimentary and assumes that names,
        /// parameters etc. are ASCII.
        ///
        /// @param str String to write.
        void WriteEscapedString(const std::string& str)
        {
            std::string::const_iterator it = str.begin();
            while (it != str.end())
            {
                char c = *it++;

                switch (c)
                {
                case '"':
                    _stream << "&quot;";
                    break;

                case '\'':
                    _stream << "&apos;";
                    break;

                case '<':
                    _stream << "&lt;";
                    break;

                case '>':
                    _stream << "&gt;";
                    break;

                case '&':
                    _stream << "&amp;";
                    break;

                default:
                    _stream << c;
                    break;
                }
            }
        }


        std::ostream& _stream;
        TestSuiteMap _testSuites;
    };
}

#endif

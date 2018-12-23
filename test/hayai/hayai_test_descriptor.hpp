#ifndef __HAYAI_TESTDESCRIPTOR
#define __HAYAI_TESTDESCRIPTOR
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#include "hayai_test.hpp"
#include "hayai_test_factory.hpp"


namespace hayai
{
    /// Parameter declaration.

    /// Describes parameter type and name.
    class TestParameterDescriptor
    {
    public:
        TestParameterDescriptor(std::string declaration,
                                std::string value)
            :   Declaration(declaration),
                Value(value)
        {

        }


        /// Declaration.
        std::string Declaration;


        /// Value.
        std::string Value;
    };


    /// Test parameters descriptor.
    class TestParametersDescriptor
    {
    private:
        /// Quoting state.
        enum QuotingState
        {
            /// Unquoted.
            Unquoted,


            /// Single quoted.
            SingleQuoted,


            /// Double quoted.
            DoubleQuoted
        };


        /// Trimmed string.

        /// @param start Start character.
        /// @param end Character one position beyond end.
        inline static std::string TrimmedString(const char* start,
                                                const char* end)
        {
            while (start < end)
            {
                if ((*start == ' ') ||
                    (*start == '\r') ||
                    (*start == '\n') ||
                    (*start == '\t'))
                    ++start;
                else
                    break;
            }

            while (end > start)
            {
                const char c = *(end - 1);

                if ((c != ' ') &&
                    (c != '\r') &&
                    (c != '\n') &&
                    (c != '\t'))
                    break;

                --end;
            }

            return std::string(start, std::string::size_type(end - start));
        }


        /// Parse comma separated parentherized value.

        /// @param separated Separated values as "(..[, ..])".
        /// @returns the individual values with white space trimmed.
        static std::vector<std::string>
            ParseCommaSeparated(const char* separated)
        {
            std::vector<std::string> result;

            if (*separated)
                ++separated;

            while ((*separated) && (*separated != ')'))
            {
                std::size_t escapeCounter = 0;
                const char* start = separated;
                QuotingState state = Unquoted;
                bool escaped = false;

                while (*separated)
                {
                    const char c = *separated++;

                    if (state == Unquoted)
                    {
                        if ((c == '"') || (c == '\''))
                        {
                            state = (c == '"' ? DoubleQuoted : SingleQuoted);
                            escaped = false;
                        }
                        else if ((c == '<') ||
                                 (c == '(') ||
                                 (c == '[') ||
                                 (c == '{'))
                            ++escapeCounter;
                        else if ((escapeCounter) &&
                                 ((c == '>') ||
                                  (c == ')') ||
                                  (c == ']') ||
                                  (c == '}')))
                            --escapeCounter;
                        else if ((!escapeCounter) &&
                                 ((c == ',') || (c == ')')))
                        {
                            result.push_back(TrimmedString(start,
                                                           separated - 1));
                            break;
                        }
                    }
                    else
                    {
                        if (escaped)
                            escaped = false;
                        else if (c == '\\')
                            escaped = true;
                        else if (c == (state == DoubleQuoted ? '"' : '\''))
                            state = Unquoted;
                    }
                }
            }

            return result;
        }


        /// Parse parameter declaration.

        /// @param raw Raw declaration.
        TestParameterDescriptor ParseDescriptor(const std::string& raw)
        {
            const char* position = raw.c_str();

            // Split the declaration into its declaration and its default
            // type.
            const char* equalPosition = NULL;
            std::size_t escapeCounter = 0;
            QuotingState state = Unquoted;
            bool escaped = false;

            while (*position)
            {
                const char c = *position++;

                if (state == Unquoted)
                {
                    if ((c == '"') || (c == '\''))
                    {
                        state = (c == '"' ? DoubleQuoted : SingleQuoted);
                        escaped = false;
                    }
                    else if ((c == '<') ||
                             (c == '(') ||
                             (c == '[') ||
                             (c == '{'))
                        ++escapeCounter;
                    else if ((escapeCounter) &&
                             ((c == '>') ||
                              (c == ')') ||
                              (c == ']') ||
                              (c == '}')))
                        --escapeCounter;
                    else if ((!escapeCounter) &&
                             (c == '='))
                    {
                        equalPosition = position;
                        break;
                    }
                }
                else
                {
                    if (escaped)
                        escaped = false;
                    else if (c == '\\')
                        escaped = true;
                    else if (c == (state == DoubleQuoted ? '"' : '\''))
                        state = Unquoted;
                }
            }

            // Construct the parameter descriptor.
            if (equalPosition)
            {
                const char* start = raw.c_str();
                const char* end = start + raw.length();

                return TestParameterDescriptor(
                    std::string(TrimmedString(start,
                                              equalPosition - 1)),
                    std::string(TrimmedString(equalPosition,
                                              end))
                );
            }
            else
                return TestParameterDescriptor(raw, std::string());
        }
    public:
        TestParametersDescriptor()
        {

        }


        TestParametersDescriptor(const char* rawDeclarations,
                                 const char* rawValues)
        {
            // Parse the declarations.
            std::vector<std::string> declarations =
                ParseCommaSeparated(rawDeclarations);

            for (std::vector<std::string>::const_iterator it =
                     declarations.begin();
                 it != declarations.end();
                 ++it)
                _parameters.push_back(ParseDescriptor(*it));

            // Parse the values.
            std::vector<std::string> values = ParseCommaSeparated(rawValues);

            std::size_t
                straightValues = (_parameters.size() > values.size() ?
                                  values.size() :
                                  _parameters.size()),
                variadicValues = 0;

            if (values.size() > _parameters.size())
            {
                if (straightValues > 0)
                    --straightValues;
                variadicValues = values.size() - _parameters.size() + 1;
            }

            for (std::size_t i = 0; i < straightValues; ++i)
                _parameters[i].Value = values[i];

            if (variadicValues)
            {
                std::stringstream variadic;

                for (std::size_t i = 0; i < variadicValues; ++i)
                {
                    if (i)
                        variadic << ", ";
                    variadic << values[straightValues + i];
                }

                _parameters[_parameters.size() - 1].Value = variadic.str();
            }
        }


        inline const std::vector<TestParameterDescriptor>& Parameters() const
        {
            return _parameters;
        }
    private:
        std::vector<TestParameterDescriptor> _parameters;
    };


    /// Test descriptor.
    class TestDescriptor
    {
    public:
        /// Initialize a new test descriptor.

        /// @param fixtureName Name of the fixture.
        /// @param testName Name of the test.
        /// @param runs Number of runs for the test.
        /// @param iterations Number of iterations per run.
        /// @param testFactory Test factory implementation for the test.
        /// @param parameters Parametrized test parameters.
        TestDescriptor(const char* fixtureName,
                       const char* testName,
                       std::size_t runs,
                       std::size_t iterations,
                       TestFactory* testFactory,
                       TestParametersDescriptor parameters,
                       bool isDisabled = false)
            :   FixtureName(fixtureName),
                TestName(testName),
                CanonicalName(std::string(fixtureName) + "." + testName),
                Runs(runs),
                Iterations(iterations),
                Factory(testFactory),
                Parameters(parameters),
                IsDisabled(isDisabled)
        {

        }


        /// Dispose of a test descriptor.
        ~TestDescriptor()
        {
            delete this->Factory;
        }


        /// Fixture name.
        std::string FixtureName;


        /// Test name.
        std::string TestName;


        /// Canonical name.

        /// As: <FixtureName>.<TestName>.
        std::string CanonicalName;


        /// Test runs.
        std::size_t Runs;


        /// Iterations per test run.
        std::size_t Iterations;


        /// Test factory.
        TestFactory* Factory;


        /// Parameters for parametrized tests
        TestParametersDescriptor Parameters;


        /// Disabled.
        bool IsDisabled;
    };
}
#endif

#ifndef __HAYAI_MAIN
#define __HAYAI_MAIN
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <errno.h>
#include <fstream>
#include <set>
#include <vector>

#include "hayai.hpp"


#if defined(_WIN32)
#    define PATH_SEPARATOR '\\'
#else
#    define PATH_SEPARATOR '/'
#endif


#define HAYAI_MAIN_FORMAT_FLAG(_desc)                                   \
    ::hayai::Console::TextGreen << _desc << ::hayai::Console::TextDefault
#define HAYAI_MAIN_FORMAT_ARGUMENT(_desc)                               \
    ::hayai::Console::TextYellow << _desc << ::hayai::Console::TextDefault
#define HAYAI_MAIN_FORMAT_ERROR(_desc)                                  \
    ::hayai::Console::TextRed << "Error:" <<                            \
        ::hayai::Console::TextDefault << " " << _desc
#define HAYAI_MAIN_USAGE_ERROR(_desc)                                   \
    {                                                                   \
        std::cerr << HAYAI_MAIN_FORMAT_ERROR(_desc) << std::endl        \
                  << std::endl;                                         \
        ShowUsage(argv[0]);                                             \
        return EXIT_FAILURE;                                            \
    }


namespace hayai
{
    /// Execution mode.
    enum MainExecutionMode
    {
        /// Run benchmarks.
        MainRunBenchmarks,


        /// List benchmarks but do not execute them.
        MainListBenchmarks
    };


    /// File outputter.
    class FileOutputter
    {
    public:
        /// File outputter.

        /// @param path Output path. Expected to be available during the life
        /// time of the outputter.
        FileOutputter(const char* path)
            :   _path(path),
                _outputter(NULL)
        {

        }


        virtual ~FileOutputter()
        {
            if (_outputter)
                delete _outputter;

            _stream.close();
        }


        /// Set up.

        /// Opens the output file for writing and initializes the outputter.
        virtual void SetUp()
        {
            _stream.open(_path,
                         std::ios_base::out |
                         std::ios_base::trunc |
                         std::ios_base::binary);
            if (_stream.bad())
            {
                std::stringstream error;
                error << "failed to open " << _path << " for writing: "
                      << strerror(errno);
                throw std::runtime_error(error.str());
            }

            _outputter = CreateOutputter(_stream);
        }


        /// Outputter.
        virtual ::hayai::Outputter& Outputter()
        {
            if (!_outputter)
                throw std::runtime_error("outputter has not been set up");

            return *_outputter;
        }
    protected:
        /// Create outputter from output stream.

        /// @param stream Output stream for the outputter.
        /// @returns the outputter for the given format.
        virtual ::hayai::Outputter* CreateOutputter(std::ostream& stream) = 0;
    private:
        const char* _path;
        std::ofstream _stream;
        ::hayai::Outputter* _outputter;
    };


#define FILE_OUTPUTTER_IMPLEMENTATION(_prefix)                          \
    class _prefix ## FileOutputter                                      \
        :   public FileOutputter                                        \
    {                                                                   \
    public:                                                             \
        _prefix ## FileOutputter(const char* path)                      \
            :   FileOutputter(path)                                     \
        {}                                                              \
    protected:                                                          \
        virtual ::hayai::Outputter* CreateOutputter(std::ostream& stream) \
        {                                                               \
            return new ::hayai::_prefix ## Outputter(stream);           \
        }                                                               \
    }


    FILE_OUTPUTTER_IMPLEMENTATION(Json);
    FILE_OUTPUTTER_IMPLEMENTATION(Console);
    FILE_OUTPUTTER_IMPLEMENTATION(JUnitXml);

#undef FILE_OUTPUTTER_IMPLEMENTATION


    /// Default main executable runner for Hayai.
    class MainRunner
    {
    public:
        MainRunner()
            :   ExecutionMode(MainRunBenchmarks),
                ShuffleBenchmarks(false),
                StdoutOutputter(NULL)
        {

        }


        ~MainRunner()
        {
            // Clean up the outputters.
            for (std::vector<FileOutputter*>::iterator it =
                     FileOutputters.begin();
                 it != FileOutputters.end();
                 ++it)
                delete *it;

            if (StdoutOutputter)
                delete StdoutOutputter;
        }


        /// Execution mode.
        MainExecutionMode ExecutionMode;


        /// Shuffle benchmarks.
        bool ShuffleBenchmarks;


        /// File outputters.
        ///
        /// Outputter will be freed by the class on destruction.
        std::vector<FileOutputter*> FileOutputters;


        /// Standard output outputter.
        ///
        /// Will be freed by the class on destruction.
        Outputter* StdoutOutputter;


        /// Parse arguments.

        /// @param argc Argument count including the executable name.
        /// @param argv Arguments.
        /// @param residualArgs Pointer to vector to hold residual arguments
        /// after parsing. If not NULL, the parser will not fail upon
        /// encounting an unknown argument but will instead add it to the list
        /// of residual arguments and return a success code. Note: the parser
        /// will still fail if an invalid value is provided to a known
        /// argument.
        /// @returns 0 on success, otherwise the exit status code to be
        /// returned from the executable.
        int ParseArgs(int argc,
                      char** argv,
                      std::vector<char*>* residualArgs = NULL)
        {
            int argI = 1;
            while (argI < argc)
            {
                char* arg = argv[argI++];
                bool argLast = (argI == argc);
                std::size_t argLen = strlen(arg);

                if (argLen == 0)
                    continue;

                // List flag.
                if ((!strcmp(arg, "-l")) || (!strcmp(arg, "--list")))
                    ExecutionMode = ::hayai::MainListBenchmarks;
                // Shuffle flag.
                else if ((!strcmp(arg, "-s")) || (!strcmp(arg, "--shuffle")))
                    ShuffleBenchmarks = true;
                // Filter flag.
                else if ((!strcmp(arg, "-f")) || (!strcmp(arg, "--filter")))
                {
                    if ((argLast) || (*argv[argI] == 0))
                        HAYAI_MAIN_USAGE_ERROR(HAYAI_MAIN_FORMAT_FLAG(arg) <<
                                    " requires a pattern to be specified");
                    char* pattern = argv[argI++];

                    ::hayai::Benchmarker::ApplyPatternFilter(pattern);
                }
                // Output flag.
                else if ((!strcmp(arg, "-o")) || (!strcmp(arg, "--output")))
                {
                    if (argLast)
                        HAYAI_MAIN_USAGE_ERROR(HAYAI_MAIN_FORMAT_FLAG(arg) <<
                                    " requires a format to be specified");
                    char* formatSpecifier = argv[argI++];

                    char* format = formatSpecifier;
                    char* path = strchr(formatSpecifier, ':');
                    if (path)
                    {
                        *(path++) = 0;
                        if (!strlen(path))
                            path = NULL;
                    }

#define ADD_OUTPUTTER(_prefix)                                          \
                    {                                                   \
                        if (path)                                       \
                            FileOutputters.push_back(                   \
                                new ::hayai::_prefix ## FileOutputter(path) \
                            );                                          \
                        else                                            \
                        {                                               \
                            if (StdoutOutputter)                        \
                                delete StdoutOutputter;                 \
                            StdoutOutputter =                           \
                                new ::hayai::_prefix ## Outputter(std::cout); \
                        }                                               \
                    }

                    if (!strcmp(format, "console"))
                        ADD_OUTPUTTER(Console)
                    else if (!strcmp(format, "json"))
                        ADD_OUTPUTTER(Json)
                    else if (!strcmp(format, "junit"))
                        ADD_OUTPUTTER(JUnitXml)
                    else
                        HAYAI_MAIN_USAGE_ERROR("invalid format: " << format);

#undef ADD_OUTPUTTER
                }
                // Console coloring flag.
                else if ((!strcmp(arg, "-c")) || (!strcmp(arg, "--color")))
                {
                    if (argLast)
                        HAYAI_MAIN_USAGE_ERROR(
                            HAYAI_MAIN_FORMAT_FLAG(arg) <<
                            " requires an argument " <<
                            "of either " << HAYAI_MAIN_FORMAT_FLAG("yes") <<
                            " or " << HAYAI_MAIN_FORMAT_FLAG("no")
                        );

                    char* choice = argv[argI++];
                    bool enabled;

                    if ((!strcmp(choice, "yes")) ||
                        (!strcmp(choice, "true")) ||
                        (!strcmp(choice, "on")) ||
                        (!strcmp(choice, "1")))
                        enabled = true;
                    else if ((!strcmp(choice, "no")) ||
                             (!strcmp(choice, "false")) ||
                             (!strcmp(choice, "off")) ||
                             (!strcmp(choice, "0")))
                        enabled = false;
                    else
                        HAYAI_MAIN_USAGE_ERROR(
                            "invalid argument to " <<
                            HAYAI_MAIN_FORMAT_FLAG(arg) <<
                            ": " << choice
                        );

                    ::hayai::Console::SetFormattingEnabled(enabled);
                }
                // Help.
                else if ((!strcmp(arg, "-?")) ||
                         (!strcmp(arg, "-h")) ||
                         (!strcmp(arg, "--help")))
                {
                    ShowUsage(argv[0]);
                    return EXIT_FAILURE;
                }
                else if (residualArgs)
                {
                    residualArgs->push_back(arg);
                }
                else
                {
                    HAYAI_MAIN_USAGE_ERROR("unknown option: " << arg);
                }
            }

            return EXIT_SUCCESS;
        }


        /// Run the selected execution mode.

        /// @returns the exit status code to be returned from the executable.
        int Run()
        {
            // Execute based on the selected mode.
            switch (ExecutionMode)
            {
            case ::hayai::MainRunBenchmarks:
                return RunBenchmarks();

            case ::hayai::MainListBenchmarks:
                return ListBenchmarks();

            default:
                std::cerr << HAYAI_MAIN_FORMAT_ERROR(
                    "invalid execution mode: " << ExecutionMode
                ) << std::endl;
                return EXIT_FAILURE;
            }
        }
    private:
        /// Run benchmarks.

        /// @returns the exit status code to be returned from the executable.
        int RunBenchmarks()
        {
            // Hook up the outputs.
            if (StdoutOutputter)
                ::hayai::Benchmarker::AddOutputter(*StdoutOutputter);

            for (std::vector< ::hayai::FileOutputter*>::iterator it =
                     FileOutputters.begin();
                 it < FileOutputters.end();
                 ++it)
            {
                ::hayai::FileOutputter& fileOutputter = **it;

                try
                {
                    fileOutputter.SetUp();
                }
                catch (std::exception& e)
                {
                    std::cerr << HAYAI_MAIN_FORMAT_ERROR(e.what()) << std::endl;
                    return EXIT_FAILURE;
                }

                ::hayai::Benchmarker::AddOutputter(fileOutputter.Outputter());
            }

            // Run the benchmarks.
            if (ShuffleBenchmarks)
            {
                std::srand(static_cast<unsigned>(std::time(0)));
                ::hayai::Benchmarker::ShuffleTests();
            }

            ::hayai::Benchmarker::RunAllTests();

            return EXIT_SUCCESS;
        }


        /// List benchmarks.

        /// @returns the exit status code to be returned from the executable.
        int ListBenchmarks()
        {
            // List out the unique benchmark names.
            std::vector<const ::hayai::TestDescriptor*> tests =
                ::hayai::Benchmarker::ListTests();
            std::vector<std::string> testNames;
            std::set<std::string> uniqueTestNames;

            for (std::vector<const ::hayai::TestDescriptor*>::iterator it =
                     tests.begin();
                 it < tests.end();
                 ++it)
            {
                if (uniqueTestNames.find((*it)->CanonicalName) !=
                    uniqueTestNames.end())
                    continue;

                testNames.push_back((*it)->CanonicalName);
                uniqueTestNames.insert((*it)->CanonicalName);
            }

            // Sort the benchmark names.
            std::sort(testNames.begin(), testNames.end());

            // Dump the list.
            for (std::vector<std::string>::iterator it = testNames.begin();
                 it < testNames.end();
                 ++it)
                std::cout << *it << std::endl;

            return EXIT_SUCCESS;
        }


        /// Show usage.

        /// @param execName Executable name.
        void ShowUsage(const char* execName)
        {
            const char* baseName = strrchr(execName, PATH_SEPARATOR);

            std::cerr << "Usage: " << (baseName ? baseName + 1 : execName)
                      << " " << HAYAI_MAIN_FORMAT_FLAG("[OPTIONS]") << std::endl
                      << std::endl

                      << "  Runs the benchmarks for this project." << std::endl
                      << std::endl

                      << "Benchmark selection options:" << std::endl
                      << "  " << HAYAI_MAIN_FORMAT_FLAG("-l") << ", "
                      << HAYAI_MAIN_FORMAT_FLAG("--list")
                      << std::endl
                      << "    List the names of all benchmarks instead of "
                      << "running them." << std::endl
                      << "  " << HAYAI_MAIN_FORMAT_FLAG("-f") << ", "
                      << HAYAI_MAIN_FORMAT_FLAG("--filter")
                      << " <" << HAYAI_MAIN_FORMAT_ARGUMENT("pattern") << ">"
                      << std::endl
                      << "    Run only the tests whose name matches one of the "
                      << "positive patterns but" << std::endl
                      << "    none of the negative patterns. '?' matches any "
                      << "single character; '*'" << std::endl
                      << "    matches any substring; ':' separates two "
                      << "patterns."
                      << std::endl

                      << "Benchmark execution options:" << std::endl
                      << "  " << HAYAI_MAIN_FORMAT_FLAG("-s") << ", "
                      << HAYAI_MAIN_FORMAT_FLAG("--shuffle")
                      << std::endl
                      << "    Randomize benchmark execution order."
                      << std::endl
                      << std::endl

                      << "Benchmark output options:" << std::endl
                      << "  " << HAYAI_MAIN_FORMAT_FLAG("-o") << ", "
                      << HAYAI_MAIN_FORMAT_FLAG("--output")
                      << " <" << HAYAI_MAIN_FORMAT_ARGUMENT("format") << ">[:"
                      << HAYAI_MAIN_FORMAT_ARGUMENT("<path>") << "]"
                      << std::endl
                      << "    Output results in a specific format. If no "
                      << "path is specified, the output" << std::endl
                      << "    will be presented on stdout. Can be specified "
                      << "multiple times to get output" << std::endl
                      << "    in different formats. The supported formats are:"
                      << std::endl
                      << std::endl
                      << "    " << HAYAI_MAIN_FORMAT_ARGUMENT("console")
                      << std::endl
                      << "      Standard console output." << std::endl
                      << "    " << HAYAI_MAIN_FORMAT_ARGUMENT("json")
                      << std::endl
                      << "      JSON." << std::endl
                      << "    " << HAYAI_MAIN_FORMAT_ARGUMENT("junit")
                      << std::endl
                      << "      JUnit-compatible XML (very restrictive.)"
                      << std::endl
                      << std::endl
                      << "    If multiple output formats are provided without "
                      << "a path, only the last" << std::endl
                      << "    provided format will be output to stdout."
                      << std::endl
                      << "  " << HAYAI_MAIN_FORMAT_FLAG("--c") << ", "
                      << HAYAI_MAIN_FORMAT_FLAG("--color") << " ("
                      << ::hayai::Console::TextGreen << "yes"
                      << ::hayai::Console::TextDefault << "|"
                      << ::hayai::Console::TextGreen << "no"
                      << ::hayai::Console::TextDefault << ")" << std::endl
                      << "    Enable colored output when available. Default "
                      << ::hayai::Console::TextGreen << "yes"
                      << ::hayai::Console::TextDefault << "." << std::endl
                      << std::endl

                      << "Miscellaneous options:" << std::endl
                      << "  " << HAYAI_MAIN_FORMAT_FLAG("-?") << ", "
                      << HAYAI_MAIN_FORMAT_FLAG("-h") << ", "
                      << HAYAI_MAIN_FORMAT_FLAG("--help") << std::endl
                      << "    Show this help information." << std::endl
                      << std::endl

                      << "hayai version: " << HAYAI_VERSION << std::endl
                      << "Clock implementation: "
                      << ::hayai::Clock::Description()
                      << std::endl;
        }
    };
}


#undef HAYAI_MAIN_FORMAT_FLAG
#undef HAYAI_MAIN_FORMAT_ARGUMENT
#undef HAYAI_MAIN_FORMAT_ERROR
#undef HAYAI_MAIN_USAGE_ERROR

#endif

#ifndef __HAYAI_CONSOLE
#define __HAYAI_CONSOLE

#include <iostream>

#if !defined(HAYAI_NO_COLOR)
#    if defined(_WIN32)
#        ifndef NOMINMAX
#            define NOMINMAX
#        endif
#        include <windows.h>
#    else
#        include <unistd.h>
#        include <cstdio>
#    endif
#endif


namespace hayai
{
    /// Static helper class for outputting to a terminal/console.
    class Console
    {
    public:
        /// Console text colors.
        enum TextColor
        {
            /// Default console color. Used for resets.
            TextDefault,

            /// Black.
            ///
            /// @warning Avoid using black unless it is absolutely necesssary.
            TextBlack,

            /// Blue.
            TextBlue,

            /// Green.
            TextGreen,

            /// Cyan.
            TextCyan,

            /// Red.
            TextRed,

            /// Purple.
            TextPurple,

            /// Yellow.
            TextYellow,

            /// White.
            ///
            /// @warning Avoid using white unless it is absolutely necessary.
            TextWhite
        };


        /// Get the singleton instance of @ref Console.

        /// @returns a reference to the singleton instance of the
        /// benchmarker execution controller.
        inline static Console& Instance()
        {
            static Console singleton;
            return singleton;
        }


        /// Test if formatting is enabled.
        inline static bool IsFormattingEnabled()
        {
            return Instance()._formattingEnabled;
        }


        /// Set whether formatting is enabled.
        inline static void SetFormattingEnabled(bool enabled)
        {
            Instance()._formattingEnabled = enabled;
        }
    private:
        inline Console()
            :   _formattingEnabled(true)
        {

        }


        bool _formattingEnabled;
    };

#if defined(_WIN32) && !defined(HAYAI_NO_COLOR) // Windows
    static inline WORD GetConsoleAttributes()
    {
        CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),
                                   &consoleInfo);
        return consoleInfo.wAttributes;
    }

    inline std::ostream& operator <<(std::ostream& stream,
                                     const Console::TextColor& color)
    {
        static const WORD defaultConsoleAttributes =
            GetConsoleAttributes();
        WORD newColor;

        if ((!Console::IsFormattingEnabled()) ||
            ((stream.rdbuf() != std::cout.rdbuf()) &&
             (stream.rdbuf() != std::cerr.rdbuf())))
            return stream;

        switch(color)
        {
            case Console::TextDefault:
                newColor = defaultConsoleAttributes;
                break;
            case Console::TextBlack:
                newColor = 0;
                break;
            case Console::TextBlue:
                newColor = FOREGROUND_BLUE;
                break;
            case Console::TextGreen:
                newColor = FOREGROUND_GREEN;
                break;
            case Console::TextCyan:
                newColor = FOREGROUND_GREEN | FOREGROUND_BLUE;
                break;
            case Console::TextRed:
                newColor = FOREGROUND_RED;
                break;
            case Console::TextPurple:
                newColor = FOREGROUND_RED | FOREGROUND_BLUE;
                break;
            case Console::TextYellow:
                newColor =
                    FOREGROUND_RED |
                    FOREGROUND_GREEN |
                    FOREGROUND_INTENSITY;
                break;
            case Console::TextWhite:
                newColor =
                    FOREGROUND_RED |
                    FOREGROUND_GREEN |
                    FOREGROUND_BLUE |
                    FOREGROUND_INTENSITY;
                break;
        }
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), newColor);
        return stream;
    }
#elif !defined(HAYAI_NO_COLOR) // Linux or others
    inline std::ostream& operator <<(std::ostream& stream,
                                     const Console::TextColor& color)
    {
        static const bool outputNoColor = isatty(fileno(stdout)) != 1;

        if ((!Console::IsFormattingEnabled()) ||
            (outputNoColor) ||
            ((stream.rdbuf() != std::cout.rdbuf()) &&
             (stream.rdbuf() != std::cerr.rdbuf())))
            return stream;

        const char* value = "";
        switch(color) {
            case Console::TextDefault:
                value = "\033[m"; break;
            case Console::TextBlack:
                value = "\033[0;30m"; break;
            case Console::TextBlue:
                value = "\033[0;34m"; break;
            case Console::TextGreen:
                value = "\033[0;32m"; break;
            case Console::TextCyan:
                value = "\033[0;36m"; break;
            case Console::TextRed:
                value = "\033[0;31m"; break;
            case Console::TextPurple:
                value = "\033[0;35m"; break;
            case Console::TextYellow:
                value = "\033[0;33m"; break;
            case Console::TextWhite:
                value = "\033[0;37m"; break;
        }
        return stream << value;
    }
#else // No color
    inline std::ostream& operator <<(std::ostream& stream,
                                     const Console::TextColor&)
    {
        return stream;
    }
#endif
}
#endif

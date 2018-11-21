#ifndef __HAYAI_DEFAULTTESTFACTORY
#define __HAYAI_DEFAULTTESTFACTORY
#include "hayai_test_factory.hpp"

namespace hayai
{
    /// Default test factory implementation.

    /// Simply constructs an instance of a the test of class @ref T with no
    /// constructor parameters.
    ///
    /// @tparam T Test class.
    template<class T>
    class TestFactoryDefault
        :   public TestFactory
    {
    public:
        /// Create a test instance with no constructor parameters.

        /// @returns a pointer to an initialized test.
        virtual Test* CreateTest()
        {
            return new T();
        }
    };
}
#endif

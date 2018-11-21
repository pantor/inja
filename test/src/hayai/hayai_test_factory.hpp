#ifndef __HAYAI_TESTFACTORY
#define __HAYAI_TESTFACTORY
#include "hayai_test.hpp"

namespace hayai
{
    /// Base class for test factory implementations.
    class TestFactory
    {
    public:
        /// Virtual destructor

        /// Has no function in the base class.
        virtual ~TestFactory()
        {

        }


        /// Creates a test instance to run.

        /// @returns a pointer to an initialized test.
        virtual Test* CreateTest() = 0;
    };
}
#endif

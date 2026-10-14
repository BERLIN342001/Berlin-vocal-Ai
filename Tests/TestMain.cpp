/*
    TestMain.cpp
    ------------
    Headless test runner. Exit code 0 = all passed.
*/

#include <cstdio>

namespace bva::test
{
    extern int runDSPTests();
    extern int runParameterTests();
}

int main()
{
    std::printf ("=== Berlin Vocal AI - Phase 1 test suite ===\n");

    int failures = 0;
    failures += bva::test::runParameterTests();
    failures += bva::test::runDSPTests();

    if (failures == 0)
        std::printf ("\nALL TESTS PASSED\n");
    else
        std::printf ("\n%d TEST FAILURE(S)\n", failures);

    return failures == 0 ? 0 : 1;
}

#include <gtest/gtest.h>

//int testArgc;
//char** testArgv;

int main ([[maybe_unused]]int argc, [[maybe_unused]]char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);

//    testArgc = argc;
//    testArgv = argv;

    return RUN_ALL_TESTS();
}

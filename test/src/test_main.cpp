// #include <filesystem>

#include "wav.hpp"

#include <gtest/gtest.h>

// namespace fs = std::filesystem;


TEST( TeleaudioTest, ParseNonExistantFile )
{
    WAV::File const f{ "this_does_not_exist.wav" };

    ASSERT_FALSE( f.valid() );
}

TEST( TeleaudioTest, ParseSimpleFile )
{
    // WAV::File const f{ "this_does_not_exist.wav" };

    // ASSERT_FALSE( f.valid() );
}

int main ( int argc, char ** argv )
{
    ::testing::InitGoogleTest( &argc, argv );

    return RUN_ALL_TESTS();
}

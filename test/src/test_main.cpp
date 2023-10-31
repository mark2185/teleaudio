#include <gtest/gtest.h>
#include <filesystem>

#include "wav.hpp"
#include "src/resources.hpp"

inline static std::filesystem::path const resources{ RESOURCES_PATH };

TEST( TeleaudioTest, ParseNonExistantFile )
{
    auto const f{ WAV::File::load( "this_does_not_exist.wav" ) };

    ASSERT_FALSE( f->valid() );
}

TEST( TeleaudioTest, ParseSimpleFile )
{
    auto const f{ WAV::File::load( ( resources / "AMAZING_clean.wav" ).string() ) };

    ASSERT_TRUE( f->valid() );
}

TEST( TeleaudioTest, CompareParsedInMemoryFileWithFileOnDisk )
{
    auto const filepath{ resources / "AMAZING_clean.wav" };

    auto const wav_file    { WAV::File::load( filepath.string() ) };
    auto const file_on_disk{ FileUtils::openFile( filepath.string(), FileUtils::FileOpenMode::ReadBinary ) };

    auto const filesize{ std::filesystem::file_size( filepath ) };
    auto buffer{ std::make_unique< std::byte[] >( filesize ) };

    ASSERT_EQ( filesize, std::fread( buffer.get(), 1, filesize, file_on_disk.get() ) );

    ASSERT_TRUE( wav_file->valid() );

    ASSERT_EQ( 0, std::memcmp( wav_file.get(), buffer.get(), filesize ) );
}

// TODO: add tests for network communication/streaming, maybe a python script that launches both

int main ( int argc, char ** argv )
{
    ::testing::InitGoogleTest( &argc, argv );

    return RUN_ALL_TESTS();
}

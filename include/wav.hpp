#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <memory>

#include <spdlog/spdlog.h>

#include "utils.hpp"

namespace WAV
{
namespace MagicBytes
{
    inline constexpr std::array< std::byte, 4 > RIFF = { std::byte{ 0x52 }, std::byte{ 0x49 }, std::byte{ 0x46 }, std::byte{ 0x46 } };
    inline constexpr std::array< std::byte, 4 > WAVE = { std::byte{ 0x57 }, std::byte{ 0x41 }, std::byte{ 0x56 }, std::byte{ 0x45 } };
    inline constexpr std::array< std::byte, 4 > fmt  = { std::byte{ 0x66 }, std::byte{ 0x6d }, std::byte{ 0x74 }, std::byte{ 0x20 } };
    inline constexpr std::array< std::byte, 4 > data = { std::byte{ 0x64 }, std::byte{ 0x61 }, std::byte{ 0x74 }, std::byte{ 0x61 } };
};

struct RiffChunk
{
    std::array< std::byte, 4 > id{ MagicBytes::RIFF };
    std::uint32_t              size;
    std::array< std::byte, 4 > format{ MagicBytes::WAVE };

    RiffChunk( std::uint32_t const size ) : size{ size } {}

    // Checks the magic bytes
    [[ nodiscard ]] bool valid() const;
};

struct FmtSubChunk
{
    std::array< std::byte, 4 > id{ MagicBytes::fmt };
    std::uint32_t              size;
    std::uint16_t              audio_format;
    std::uint16_t              num_channels;
    std::uint32_t              sample_rate;
    std::uint32_t              byte_rate;
    std::uint16_t              block_align;
    std::uint16_t              bits_per_sample;

    // Checks:
    //   - magic bytes
    //   - does block align make sense
    //   - does byte   rate make sense
    [[ nodiscard ]] bool valid() const;
};

struct DataSubChunk
{
    std::array< std::byte, 4 > id{ MagicBytes::data };
    std::uint32_t              size;

    // DataSubChunk is the owner of the data,
    // this is the C++ legal equivalent of the "C struct hack"
    std::byte data[1];

    // Checks the magic bytes
    [[ nodiscard ]] bool valid() const;
};

struct File
{
    RiffChunk    riff;
    FmtSubChunk  format;
    DataSubChunk data;

    // Loads up a .wav file
    static std::unique_ptr< File > load( std::string_view const filename )
    {
        auto const file_handle{ FileUtils::openFile( filename, FileUtils::FileOpenMode::ReadBinary ) };
        if ( !file_handle )
        {
            spdlog::error( "Cannot open file '{}' for loading WAV file.", filename );
            return nullptr;
        }
        auto const file_size{ std::filesystem::file_size( filename ) };
        auto buffer{ std::make_unique< std::byte[] >( std::filesystem::file_size( filename ) ) };
        auto const bytes_read{ std::fread( buffer.get(), 1, file_size, file_handle.get() ) };

        if ( bytes_read != file_size )
        {
            spdlog::error( "Reading from {} failed, read {} bytes, but should have read {}.", filename, bytes_read, file_size );
            return nullptr;
        }

        std::unique_ptr< File > ret;
        ret.reset( reinterpret_cast< File * >( buffer.release() ) );
        return ret;
    }

    [[ nodiscard ]] std::uint32_t size_in_bytes() const
    {
        return static_cast< std::uint32_t >( riff.id.size()      )
             + static_cast< std::uint32_t >( sizeof( riff.size ) )
             + riff.size;
    }

    // Checks the validity of all subchunks
    [[ nodiscard ]] bool valid() const;

    // Writes to given path
    [[ nodiscard ]] bool write( std::string_view path ) const;
};

} // namespace WAV

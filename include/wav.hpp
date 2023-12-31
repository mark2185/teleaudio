#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
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

    RiffChunk() = default;
    RiffChunk( std::uint32_t const size ) : size{ size } {}

    // Checks the magic bytes
    [[ nodiscard ]] bool valid() const;
};

struct FmtSubChunk
{
    std::array< std::byte, 4 > subchunk1_id{ MagicBytes::fmt };
    std::uint32_t              subchunk1_size;
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
    std::array< std::byte, 4 >     subchunk2_id{ MagicBytes::data };
    std::uint32_t                  subchunk2_size;

    // DataSubChunk is the owner of the data
    std::unique_ptr< std::byte[] > data;

    // Checks the magic bytes
    [[ nodiscard ]] bool valid() const;

    // Deep copy
    [[ nodiscard ]] DataSubChunk copy() const;
};

struct File
{
    RiffChunk    riff;
    FmtSubChunk  format;
    DataSubChunk data;

    File() = default;

    File( FmtSubChunk const metadata, DataSubChunk && audiodata )
        : format{ metadata }, data{ std::move( audiodata ) }
    {}

    // Takes ownership of the raw data
    File( FmtSubChunk metadata, std::byte * raw_data, std::uint32_t subchunk2_size );

    // Loads up a .wav file
    File( std::string_view filename );

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

    // Layouts the file in memory as it would be when written onto a disk
    Utils::OwningBuffer copyInMemory() const;
};

} // namespace WAV

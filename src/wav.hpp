#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>

#include <spdlog/spdlog.h>

namespace WAV
{
namespace MagicBytes
{
    inline constexpr std::array< std::byte, 4 > RIFF = { std::byte{ 0x52 }, std::byte{ 0x49 }, std::byte{ 0x46 }, std::byte{ 0x46 } };
    inline constexpr std::array< std::byte, 4 > WAVE = { std::byte{ 0x57 }, std::byte{ 0x41 }, std::byte{ 0x56 }, std::byte{ 0x45 } };
    inline constexpr std::array< std::byte, 4 > fmt  = { std::byte{ 0x66 }, std::byte{ 0x6d }, std::byte{ 0x74 }, std::byte{ 0x20 } };
    inline constexpr std::array< std::byte, 4 > data = { std::byte{ 0x64 }, std::byte{ 0x61 }, std::byte{ 0x74 }, std::byte{ 0x61 } };
};

struct OwningBuffer
{
    std::unique_ptr< std::byte[] > data;
    std::size_t                    size;

    OwningBuffer( std::size_t const size )
        : data{ std::make_unique< std::byte[] >( size ) }, size{ size }
    {}

    std::byte       * get()       { return data.get(); }
    std::byte const * get() const { return data.get(); }
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
    RiffChunk    riff{};
    FmtSubChunk  format{};
    DataSubChunk data{};

    File() = default;

    File( FmtSubChunk const metadata, DataSubChunk && audiodata )
        : format{ metadata }, data{ std::move( audiodata ) }
    {}

    // Takes ownership of the raw data
    File( FmtSubChunk const metadata, std::byte * rawData, std::uint32_t const subchunk2_size )
    {
        data.data.reset( rawData );
        data.subchunk2_size = subchunk2_size;

        format = metadata;

        // the subchunk sizes denote the size of the _rest of the current chunk_
        auto const bytes_before_subchunk_size{ 8 };
        riff.size = static_cast< std::uint32_t >( MagicBytes::RIFF.size() )
                  + bytes_before_subchunk_size + format.subchunk1_size
                  + bytes_before_subchunk_size +   data.subchunk2_size;
    }

    // Loads up a .wav file
    File( std::string_view const filename );

    [[ nodiscard ]] std::uint32_t size_in_bytes() const { return riff.size + 8; }

    // Checks the validity of all subchunks
    [[ nodiscard ]] bool valid() const;

    // Writes to given path
    [[ nodiscard ]] bool write( std::string_view const path ) const;

    // Layouts the file in memory as it would be when written onto a disk
    OwningBuffer copyInMemory() const
    {
        OwningBuffer buffer{ size_in_bytes() };

        auto output_iterator{ buffer.get() };

        output_iterator = std::copy_n( reinterpret_cast< std::byte const * >( &riff           ), sizeof( riff   )                                           , output_iterator );
        output_iterator = std::copy_n( reinterpret_cast< std::byte const * >( &format         ), sizeof( format )                                           , output_iterator );
        output_iterator = std::copy_n( reinterpret_cast< std::byte const * >( &data           ), sizeof( data.subchunk2_id ) + sizeof( data.subchunk2_size ), output_iterator );
        output_iterator = std::copy_n( reinterpret_cast< std::byte const * >( data.data.get() ), data.subchunk2_size                                        , output_iterator );

        return buffer;
    }
};

} // namespace WAV

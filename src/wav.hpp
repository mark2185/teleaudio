#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include "spdlog/spdlog.h"

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
    [[ nodiscard ]] bool valid() const
    {
        auto const magic_bytes_match
        {
            std::memcmp(     id.data(), MagicBytes::RIFF.data(), MagicBytes::RIFF.size() ) == 0
         && std::memcmp( format.data(), MagicBytes::WAVE.data(), MagicBytes::WAVE.size() ) == 0
        };
        return magic_bytes_match;
    }
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
    [[ nodiscard ]] bool valid() const
    {
        auto const magic_bytes_match   { std::memcmp( subchunk1_id.data(), MagicBytes::fmt.data(), MagicBytes::fmt.size() ) == 0 };
        auto const expected_byte_rate  { sample_rate * num_channels * bits_per_sample / 8 };
        auto const expected_block_align{ num_channels * bits_per_sample / 8 };
        return magic_bytes_match
            && expected_byte_rate   == byte_rate
            && expected_block_align == block_align;
    }

};

struct DataSubChunk
{
    std::array< std::byte, 4 >     subchunk2_id{ MagicBytes::data };
    std::uint32_t                  subchunk2_size;
    // DataSubChunk is the owner of the data
    std::unique_ptr< std::byte[] > data;

    // Checks the magic bytes
    [[ nodiscard ]] bool valid() const
    {
        auto const magic_bytes_match{ std::memcmp( subchunk2_id.data(), MagicBytes::data.data(), MagicBytes::data.size() ) == 0 };
        return magic_bytes_match;
    }

    // Deep copy
    DataSubChunk copy() const
    {
        auto buffer{ std::make_unique< std::byte[] >( subchunk2_size ) };
        std::copy_n( data.get(), subchunk2_size, buffer.get() );

        return DataSubChunk{ subchunk2_id, subchunk2_size, std::move( buffer ) };
    }
};

struct File
{
    RiffChunk    riff{};
    FmtSubChunk  format{};
    DataSubChunk data{};

    std::uint32_t size_in_bytes() const
    {
        return riff.size + 8;
    }

    // Checks the validity of all subchunks
    [[ nodiscard ]] bool valid() const
    {
        auto const riffValid   { riff.valid() };
        auto const formatValid { format.valid() };
        auto const dataValid   { data.valid() };

        auto const expected_chunk_size{ 4 + ( 8 + format.subchunk1_size ) + ( 8 + data.subchunk2_size ) };

        return riffValid && formatValid && dataValid && expected_chunk_size;
    }

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

        riff.size = 4 + ( 8 + format.subchunk1_size ) + ( 8 + data.subchunk2_size );
    }

    // Loads up a .wav file
    File( std::string_view const filename )
    {
        auto const file_handle{ FileUtils::openFile( filename, FileUtils::FileOpenMode::ReadBinary ) };
        if ( !file_handle )
        {
            return;
        }

        {
            auto       buffer{ std::make_unique< std::byte[] >( sizeof( RiffChunk ) )                };
            auto const res   { std::fread( buffer.get(), 1, sizeof( RiffChunk ), file_handle.get() ) };
            if ( res != sizeof( RiffChunk ) )
            {
                spdlog::error( "Failed reading riff chunk, read {} bytes, but should have read {} bytes.", res, sizeof( WAV::RiffChunk ) );
                return;
            }
            riff = *reinterpret_cast< RiffChunk * >( buffer.get() );
        }
        {
            auto       buffer{ std::make_unique< std::byte[] >( sizeof( FmtSubChunk ) )                };
            auto const res   { std::fread( buffer.get(), 1, sizeof( FmtSubChunk ), file_handle.get() ) };
            if ( res != sizeof( FmtSubChunk ) )
            {
                spdlog::error( "Failed reading format subchunk, read {} bytes, but should have read {} bytes.", res, sizeof( WAV::FmtSubChunk ) );
                return;
            }
            format = *reinterpret_cast< FmtSubChunk * >( buffer.get() );
        }
        {
            {
                // reading only the id and datasize
                std::array< std::byte, 8 > buffer;
                auto const res{ std::fread( buffer.data(), 1, buffer.size(), file_handle.get() ) };
                if ( res != buffer.size() )
                {
                    spdlog::error( "Failed reading data subchunk info, read {} bytes, but should have read {} bytes.", res, buffer.size() );
                    return;
                }

                // copy magic bytes
                std::copy_n( buffer.data(), data.subchunk2_id.size(), data.subchunk2_id.data() );

                // copy the size of the raw data
                data.subchunk2_size = *reinterpret_cast< std::uint32_t * >( buffer.data() + 4 );
            }
            {
                auto       buffer{ std::make_unique< std::byte[] >( data.subchunk2_size )                };
                auto const res   { std::fread( buffer.get(), 1, data.subchunk2_size, file_handle.get() ) };
                if ( res != data.subchunk2_size )
                {
                    spdlog::error( "Failed reading raw data chunk, read {} bytes, but should have read {} bytes.", res, data.subchunk2_size );
                }
                data.data.swap( buffer );
            }
        }

        if ( !valid() )
        {
            spdlog::error( "Loaded file from '{}' is not valid.", filename );
        }
    }

    [[ nodiscard ]] bool write( std::string_view const path ) const
    {
        auto total_bytes_written{ 0U };

        auto const file_handle{ FileUtils::openFile( path, FileUtils::FileOpenMode::WriteBinary ) };
        if ( !file_handle )
        {
            spdlog::error( "Cannot open output file '{}' for writing.", path );
            return false;
        }

        // writing the data from the stack
        {
            auto const   riffWrite{ std::fwrite( &riff  , 1, sizeof( riff   ), file_handle.get() ) };
            auto const formatWrite{ std::fwrite( &format, 1, sizeof( format ), file_handle.get() ) };

            auto const dataBytes{ sizeof( data.subchunk2_id ) + sizeof( data.subchunk2_size ) };
            auto const dataWrite{ std::fwrite( &data, 1, dataBytes, file_handle.get() )       };

            if
            (
                riffWrite   != sizeof( riff )
             || formatWrite != sizeof( format )
             || dataWrite   != dataBytes
            )
            {
                spdlog::error( "Failed in writing the metadata of the .wav file." );
                return false;
            }

            total_bytes_written += riffWrite + formatWrite + dataWrite;
        }
        // writing the raw audio data
        {
            auto const res{ std::fwrite( data.data.get(), 1, data.subchunk2_size, file_handle.get() ) };
            if ( res != data.subchunk2_size )
            {
                spdlog::error( "Failed writing the raw audio data, written {} bytes, but should have written {}.", res );
                return false;
            }
            total_bytes_written += res;
        }

        spdlog::info( "Written {} bytes to '{}'", total_bytes_written, path );
        return true;
    }
};

} // namespace WAV

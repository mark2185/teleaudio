#include <array>
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
    // TODO: set default values to magic bytes for easier generating of a semi-valid chunk
    std::array< std::byte, 4 > id;
    std::uint32_t              size;
    std::array< std::byte, 4 > format;

    [[ nodiscard ]] bool valid() const
    {
        auto const magic_bytes_match
        {
            std::memcmp(     id.data(), MagicBytes::RIFF.data(), MagicBytes::RIFF.size() ) == 0
         && std::memcmp( format.data(), MagicBytes::WAVE.data(), MagicBytes::WAVE.size() ) == 0
        };
        return magic_bytes_match;
    }

    // for easier generation before writing to a file
    void addMagicBytes()
    {
        id = MagicBytes::RIFF;
        format = MagicBytes::WAVE;
    }

} /*__attribute__((packed))*/; // TODO: this prevents spdlog from pritning `size`

struct FmtSubChunk
{
    std::array< std::byte, 4 > subchunk1_id;
    std::uint32_t subchunk1_size;
    std::uint16_t audio_format;
    std::uint16_t num_channels;
    std::uint32_t sample_rate;
    std::uint32_t byte_rate;
    std::uint16_t block_align;
    std::uint16_t bits_per_sample;

    [[ nodiscard ]] bool valid() const
    {
        auto const magic_bytes_match{ std::memcmp( subchunk1_id.data(), MagicBytes::fmt.data(), MagicBytes::fmt.size() ) == 0 };
        auto const expected_byte_rate{ sample_rate * num_channels * bits_per_sample / 8 };
        auto const expected_block_align{ num_channels * bits_per_sample / 8 };
        return magic_bytes_match
            && expected_byte_rate   == byte_rate
            && expected_block_align == block_align;
    }

    void addMagicBytes()
    {
        subchunk1_id = MagicBytes::fmt;
    }

} __attribute__((packed));

struct DataSubchunk
{
    std::array< std::byte, 4 > subchunk2_id;
    std::uint32_t subchunk2_size;
    std::byte const * data;

    [[ nodiscard ]] bool valid() const
    {
        auto const magic_bytes_match{ std::memcmp( subchunk2_id.data(), MagicBytes::data.data(), MagicBytes::data.size() ) == 0 };
        return magic_bytes_match;
    }

    void addMagicBytes()
    {
        subchunk2_id = MagicBytes::data;
    }

} __attribute__((packed));

struct File
{
    RiffChunk riff;
    FmtSubChunk format;
    DataSubchunk data;

    std::uint32_t size_in_bytes() const
    {
        return riff.size + 8;
    }
    // FileUtils::FilePtr file_handle;

    [[ nodiscard ]] bool valid() const
    {
        auto const riffValid   { riff.valid() };
        auto const formatValid { format.valid() };
        auto const dataValid   { data.valid() };
        return riffValid && formatValid && dataValid;
    }


    // TODO: rule of five
    // TODO: deep copy
    // ~File() = default;
    // File          ( File const & ) = delete;
    // File operator=( File const & ) = delete;

    // spdlog::info( "riff size {}, chunk1 size {}, chunk2 size: {}", song.riff.size, song.format.subchunk1_size, song.data.subchunk2_size );
    // spdlog::info( "File size in bytes: {}", song.size_in_bytes() );
    // if ( !song.valid() )
    // {
        // if ( !song.riff.valid() )
        // {
            // spdlog::error( "RIFF not valid!" );
        // }
        // if ( !song.format.valid() )
        // {
            // spdlog::error( "Format not valid!" );
        // }
        // if ( !song.data.valid() )
        // {
            // spdlog::error( "Data not valid!" );
        // }
        // spdlog::error( "Failed to open song '{}'", file.string() );
        // return Status::OK;
    // }

    File() = default;

    File( std::string_view const filename )
    {
        auto file_handle{  FileUtils::openFile( filename, FileUtils::FileOpenMode::ReadBinary ) };
        if ( !file_handle )
        {
            return;
        }

        {
            auto buffer{ std::make_unique< std::byte[] >( sizeof( RiffChunk ) ) };
            auto const res{ std::fread( buffer.get(), 1, sizeof( RiffChunk ), file_handle.get() ) };
            if ( res != sizeof( RiffChunk ) )
            {
                spdlog::error( "Failed reading riff chunk, read {} bytes, but should have read {} bytes.", res, sizeof( WAV::RiffChunk ) );
            }
            riff = *reinterpret_cast< RiffChunk * >( buffer.get() );
        }
        {
            auto buffer{ std::make_unique< std::byte[] >( sizeof( FmtSubChunk ) ) };
            auto const res{ std::fread( buffer.get(), 1, sizeof( FmtSubChunk ), file_handle.get() ) };
            if ( res != sizeof( FmtSubChunk ) )
            {
                spdlog::error( "Failed reading format chunk, read {} bytes, but should have read {} bytes.", res, sizeof( WAV::FmtSubChunk ) );
            }
            format = *reinterpret_cast< FmtSubChunk * >( buffer.get() );
        }
        {
            // reading only the id and datasize
            auto buffer{ std::make_unique< std::byte[] >( 8 ) };
            auto const res{ std::fread( buffer.get(), 1, 8, file_handle.get() ) };
            if ( res != 8 )
            {
                spdlog::error( "Failed reading data chunk, read {} bytes, but should have read {} bytes.", res, 8 );
            }
            data.subchunk2_id = { buffer[0], buffer[1], buffer[2], buffer[3] };
            data.subchunk2_size = *reinterpret_cast< std::uint32_t * >( buffer.get() + 4 );
            buffer = std::make_unique< std::byte[] >( data.subchunk2_size );
            auto const rawDataRes{ std::fread( buffer.get(), 1, data.subchunk2_size, file_handle.get() ) };
            if ( rawDataRes != data.subchunk2_size )
            {
                spdlog::error( "Failed reading raw data chunk, read {} bytes, but should have read {} bytes.", rawDataRes, 666 );
            }
            data.data = buffer.release();
        }
        if ( riff.size != ( 4 + ( 8 + format.subchunk1_size ) + ( 8 + data.subchunk2_size ) ) )
        {
            spdlog::error( "Header does not match the math" );
            spdlog::error( "{} vs {}", riff.size, ( 4 + ( 8 + format.subchunk1_size ) + ( 8 + data.subchunk2_size ) ) );
        }
    }

    void addMagicBytes()
    {
        riff.addMagicBytes();
        format.addMagicBytes();
        data.addMagicBytes();
    }

    [[ nodiscard ]] bool write( std::string_view const path ) const
    {
        auto const fout{ FileUtils::openFile( path, FileUtils::FileOpenMode::WriteBinary ) };
        if ( !fout )
        {
            spdlog::error( "Cannot open output file" );
            return false;
        }

        // writing the data from the stack
        {
            auto const riffWrite{ std::fwrite( &riff, 1, sizeof( riff ), fout.get() ) };
            auto const formatWrite{ std::fwrite( &format, 1, sizeof( format ), fout.get() ) };

            auto const dataBytes{ sizeof( data.subchunk2_id ) + sizeof( data.subchunk2_size ) };
            auto const dataWrite{ std::fwrite( &data, 1, dataBytes, fout.get() ) };

            if
            (
                riffWrite   != sizeof( riff )
             || formatWrite != sizeof( format )
             || dataWrite   != dataBytes
            )
            {
                spdlog::error( "Failed in writing the stack based data of the wav file" );
                return false;
            }

        }
        // writing the data from the heap, i.e. raw audio data
        {
            auto const ret{ std::fwrite( data.data, 1, data.subchunk2_size, fout.get() ) };
            if ( ret != data.subchunk2_size )
            {
                spdlog::error( "Failed in writing the stack based data of the wav file" );
                return false;
            }
        }

        return true;
    }
} /*__attribute__((packed))*/; // TODO: msvc

inline File constructPlaceholderWaveFile( FmtSubChunk const metadata, std::byte * rawData, std::uint32_t size )
{
    File ret;

    ret.data.data           = rawData;
    ret.data.subchunk2_size = size;

    ret.format = metadata;
    // ret.format.subchunk1_size = 16;

    ret.riff.size = 4 + ( 8 + ret.format.subchunk1_size ) + ( 8 + ret.data.subchunk2_size );

    ret.addMagicBytes();

    if ( !ret.valid() )
    {
        spdlog::info( "Placeholder file is not valid" );
    }
    // assert( ret.valid() );
    return ret;
}

} // namespace WAV

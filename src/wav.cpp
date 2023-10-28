#include "wav.hpp"
#include <algorithm>

#include "utils.hpp"

namespace WAV
{
    // validations
    bool RiffChunk::valid() const
    {
        auto const magic_bytes_match
        {
            std::memcmp(     id.data(), MagicBytes::RIFF.data(), MagicBytes::RIFF.size() ) == 0
         && std::memcmp( format.data(), MagicBytes::WAVE.data(), MagicBytes::WAVE.size() ) == 0
        };
        return magic_bytes_match;
    }

    bool FmtSubChunk::valid() const
    {
        auto const magic_bytes_match   { std::memcmp( subchunk1_id.data(), MagicBytes::fmt.data(), MagicBytes::fmt.size() ) == 0 };
        auto const expected_byte_rate  { sample_rate * num_channels * bits_per_sample / 8 };
        auto const expected_block_align{ num_channels * bits_per_sample / 8 };
        return magic_bytes_match
            && expected_byte_rate   == byte_rate
            && expected_block_align == block_align;
    }

    bool DataSubChunk::valid() const
    {
        auto const magic_bytes_match{ std::memcmp( subchunk2_id.data(), MagicBytes::data.data(), MagicBytes::data.size() ) == 0 };
        return magic_bytes_match;
    }

    bool File::valid() const
    {
        auto const riffValid   { riff.valid() };
        auto const formatValid { format.valid() };
        auto const dataValid   { data.valid() };

        // the subchunk sizes denote the size of the _rest of the current chunk_
        auto const bytes_before_subchunk_size{ 8 };
        auto const expected_chunk_size
        {
            4 +
            ( bytes_before_subchunk_size + format.subchunk1_size ) +
            ( bytes_before_subchunk_size +   data.subchunk2_size )
        };

        return riffValid && formatValid && dataValid && expected_chunk_size;
    }

    DataSubChunk DataSubChunk::copy() const
    {
        auto buffer{ std::make_unique< std::byte[] >( subchunk2_size ) };
        std::copy_n( data.get(), subchunk2_size, buffer.get() );

        return DataSubChunk{ subchunk2_id, subchunk2_size, std::move( buffer ) };
    }

    File::File( FmtSubChunk const metadata, std::byte * raw_data, std::uint32_t const subchunk2_size )
    {
        data.data.reset( raw_data );
        data.subchunk2_size = subchunk2_size;

        format = metadata;

        // the subchunk sizes denote the size of the _rest of the current chunk_
        auto const bytes_before_subchunk_size{ 8 };
        riff.size = static_cast< std::uint32_t >( MagicBytes::RIFF.size() )
                  + bytes_before_subchunk_size + format.subchunk1_size
                  + bytes_before_subchunk_size +   data.subchunk2_size;
    }

    File::File( std::string_view const filename )
    {
        auto const file_handle{ FileUtils::openFile( filename, FileUtils::FileOpenMode::ReadBinary ) };
        if ( !file_handle )
        {
            return;
        }

        // TODO: possible optimization - mmap
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
                // reading the raw samples
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

    bool File::write( std::string_view const path ) const
    {
        auto const file_handle{ FileUtils::openFile( path, FileUtils::FileOpenMode::WriteBinary ) };
        if ( !file_handle )
        {
            spdlog::error( "Cannot open output file '{}' for writing.", path );
            return false;
        }

        auto const buffer{ copyInMemory() };
        auto const res{ std::fwrite( buffer.get(), 1, buffer.size, file_handle.get() ) };
        if ( res != buffer.size )
        {
            spdlog::error( "Writing to {} failed, written {} bytes, but should have written {}.", path, res, buffer.size );
            return false;
        }

        spdlog::info( "Written {} bytes to '{}'", res, path );
        return true;
    }

    // TODO: this is unnecessary overhead, we should construct WAV::File serialized on heap,
    // not have it both partially stack and heap based
    Utils::OwningBuffer File::copyInMemory() const
    {
        Utils::OwningBuffer buffer{ size_in_bytes() };

        auto output_iterator{ buffer.get() };

        output_iterator = std::copy_n( reinterpret_cast< std::byte const * >( &riff           ), sizeof( riff   )                                           , output_iterator );
        output_iterator = std::copy_n( reinterpret_cast< std::byte const * >( &format         ), sizeof( format )                                           , output_iterator );
        output_iterator = std::copy_n( reinterpret_cast< std::byte const * >( &data           ), sizeof( data.subchunk2_id ) + sizeof( data.subchunk2_size ), output_iterator );
        output_iterator = std::copy_n( reinterpret_cast< std::byte const * >( data.data.get() ), data.subchunk2_size                                        , output_iterator );

        return buffer;
    }

} // namespace WAV

#include "wav.hpp"
#include <algorithm>
#include <filesystem>

#include "utils.hpp"

namespace WAV
{
    bool RiffChunk::valid() const
    {
        auto const magic_bytes_match{ id == MagicBytes::RIFF && format == MagicBytes::WAVE };
        return magic_bytes_match;
    }

    bool FmtSubChunk::valid() const
    {
        auto const bits_in_byte        { 8                                                           };
        auto const magic_bytes_match   { id == MagicBytes::fmt                                       };
        auto const expected_byte_rate  { sample_rate * num_channels * bits_per_sample / bits_in_byte };
        auto const expected_block_align{ num_channels * bits_per_sample / bits_in_byte               };
        return magic_bytes_match
            && expected_byte_rate   == byte_rate
            && expected_block_align == block_align;
    }

    bool DataSubChunk::valid() const
    {
        auto const magic_bytes_match{ id == MagicBytes::data };
        return magic_bytes_match;
    }

    bool File::valid() const
    {
        auto const riff_valid  { riff.valid()   };
        auto const format_valid{ format.valid() };
        auto const data_valid  { data.valid()   };

        auto const expected_chunk_size
        {
            sizeof( riff.size ) +
            sizeof( format.id ) + sizeof( format.size ) + format.size +
            sizeof( data.id   ) + sizeof( data.size   ) + data.size
        };

        return riff_valid && format_valid && data_valid && expected_chunk_size;
    }

    bool File::write( std::string_view const path ) const
    {
        auto const file_handle{ FileUtils::openFile( path, FileUtils::FileOpenMode::WriteBinary ) };
        if ( !file_handle )
        {
            spdlog::error( "Cannot open output file '{}' for writing.", path );
            return false;
        }

        auto const res{ std::fwrite( this, 1, size_in_bytes(), file_handle.get() ) };
        if ( res != size_in_bytes() )
        {
            spdlog::error( "Writing to {} failed, written {} bytes, but should have written {}.", path, res, size_in_bytes() );
            return false;
        }

        spdlog::info( "Written {} bytes to '{}'", res, path );
        return true;
    }

} // namespace WAV

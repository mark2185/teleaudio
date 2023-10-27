#include "audio_client.hpp"

#include <spdlog/spdlog.h>

#include "audio_server.hpp"
#include "wav.hpp"

#include "utils.hpp"

#ifdef _WIN32
#include <Windows.h>
#include <MMSystem.h>
#endif

namespace
{
    WAV::FmtSubChunk parseMetadata( Teleaudio::AudioMetadata const metadata )
    {
        auto const pulse_code_modulation{ 1 };
        auto const pulse_code_modulation_chunk_size{ 16 };
        return
        {
            .subchunk1_id    = WAV::MagicBytes::fmt,
            .subchunk1_size  = pulse_code_modulation_chunk_size,
            .audio_format    = pulse_code_modulation,
            .num_channels    = static_cast< std::uint16_t >( metadata.channels()              ),
            .sample_rate     =                             ( metadata.samplerate()            ),
            .byte_rate       =                             ( metadata.averagebytespersecond() ),
            .block_align     = static_cast< std::uint16_t >( metadata.blockalign()            ),
            .bits_per_sample = static_cast< std::uint16_t >( metadata.bitspersample()         )
        };
    }
}

namespace Teleaudio
{
    std::string AudioClient::List( std::string_view const directory ) const
    {
        grpc::ClientContext context;

        Directory request;
        request.set_path( std::string{ directory } );

        CmdOutput response;

        grpc::Status const status{ stub_->List( &context, request, &response ) };

        if ( !status.ok() )
        {
            spdlog::error( "'ls' failed with error: {}", status.error_message() );
            return "";
        }

        return response.text();
    }

    std::optional< WAV::File > AudioClient::receiveFile( std::string_view const filename ) const
    {
        grpc::ClientContext context;

        File request;
        request.set_name( filename.data() );

        std::unique_ptr< grpc::ClientReader< AudioData > > reader{ stub_->Download( &context, request ) };

        // reading metadata first
        AudioData data;
        reader->Read( &data );

        auto const metadata{ data.metadata() };

        spdlog::info( "Metadata: {}ch {}Hz {}bps", data.metadata().channels(), data.metadata().samplerate(), data.metadata().bitspersample() );

        auto const raw_data_size  { static_cast< std::uint32_t >( data.metadata().rawdatasize() ) };
        auto       raw_data_buffer{ std::make_unique< std::byte[] >( raw_data_size )              };
        std::uint32_t bytes_read{};
        // reading the raw audio data
        while ( reader->Read( &data ) )
        {   
            auto const payload_size{ static_cast< std::uint32_t >( data.rawdata().size() ) };
            std::copy_n
            (
                reinterpret_cast< std::byte * >( data.mutable_rawdata()->data() ),
                payload_size,
                raw_data_buffer.get() + bytes_read
            );
            bytes_read += payload_size;
        }
        if ( bytes_read != raw_data_size )
        {
            spdlog::error( "Read {} bytes, but raw data size is {}", bytes_read, raw_data_size );
        }

        grpc::Status const status{ reader->Finish() };
        if ( !status.ok() )
        {
            spdlog::error( "Error while downloading the file '{}', error: {}", filename, status.error_message() );
            return std::nullopt;
        }

        auto * rawdata{ raw_data_buffer.release() };

        WAV::File file
        {
            parseMetadata( metadata ),
            rawdata, // takes ownership
            raw_data_size
        };

        if ( !file.valid() )
        {
            spdlog::error( "Received file is not valid!" );
            return std::nullopt;
        }

        return file;
    }


    bool AudioClient::Play( [[ maybe_unused ]] std::string_view const file ) const
    {
#ifdef _WIN32
        auto const wav_file{ receiveFile( file ) };
        if ( !wav_file.has_value() )
        {
            return false;
        }

        if ( !wav_file->valid() )
        {
            spdlog::error( "Cannot play file {}, it is not a supported .WAV file", file );
        }
        auto const buffer{ wav_file->constructInMemory() };
        auto const status{ PlaySound( reinterpret_cast< char * >( buffer.data.get() ), NULL, SND_MEMORY | SND_SYNC ) };
        if ( !status )
        {
            spdlog::error( "Failed to play sound for some reason" );
        }
        return true;
#else
        spdlog::warn( "Playing is not supported on non-Windows OS." );
        return false;
#endif
    }

    bool AudioClient::Download( std::string_view const file, std::string_view const output_path ) const
    {
        auto const wav_file{ receiveFile( file ) };
        if ( !wav_file.has_value() )
        {
            return false;
        }
        if ( !wav_file->write( output_path ) )
        {
            spdlog::error( "Writing file to {} failed", output_path );
            return false;
        }
        return true;
    }
} // namespace Teleaudio

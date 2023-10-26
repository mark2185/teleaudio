#include "audio_client.hpp"

#include <spdlog/spdlog.h>
#include "wav.hpp"
#include "audio_server.hpp"

namespace Teleaudio
{
    WAV::FmtSubChunk parseMetadata( AudioMetadata const metadata )
    {
        return
        {
            .subchunk1_id    = WAV::MagicBytes::fmt,
            .subchunk1_size  = 16, // fixed
            .audio_format    = 1,  // fixed, denotes PCM
            .num_channels    = static_cast< std::uint16_t >( metadata.channels()              ),
            .sample_rate     =                             ( metadata.samplerate()            ),
            .byte_rate       =                             ( metadata.averagebytespersecond() ),
            .block_align     = static_cast< std::uint16_t >( metadata.blockalign()            ),
            .bits_per_sample = static_cast< std::uint16_t >( metadata.bitspersample()         )
        };
    }

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

    bool AudioClient::Play( std::string_view const file ) const
    {
        // TODO: extract into "receiveFile" so it can be reused for downloading
        grpc::ClientContext context;

        File request;
        request.set_name( file.data() );

        std::unique_ptr< grpc::ClientReader< AudioData > > reader{ stub_->Play( &context, request ) };

        // reading metadata first
        AudioData data;
        reader->Read( &data );

        spdlog::info( "Metadata: {}ch {}Hz {}bps", data.metadata().channels(), data.metadata().samplerate(), data.metadata().bitspersample() );

        auto const metadata{ data.metadata() };

        // reading the raw audio data
        while ( reader->Read( &data ) )
        {
            // TODO: test with a huge file
        }

        grpc::Status const status{ reader->Finish() };
        if ( !status.ok() )
        {
            spdlog::error( "Error while playing the file '{}', error: {}", file, status.error_message() );
            return false;
        }

        // TODO: play the actual file on a speaker
        return true;
    }

    bool AudioClient::Download( std::string_view const file, std::string_view const output_path ) const
    {
        grpc::ClientContext context;

        File request;
        request.set_name( file.data() );

        std::unique_ptr< grpc::ClientReader< AudioData > > reader{ stub_->Download( &context, request ) };

        // reading metadata first
        AudioData data;
        reader->Read( &data );

        auto const metadata{ data.metadata() };

        spdlog::info( "Metadata: {}ch {}Hz {}bps", data.metadata().channels(), data.metadata().samplerate(), data.metadata().bitspersample() );

        // reading the raw audio data
        while ( reader->Read( &data ) )
        {
            spdlog::info( "Reading loop" );
            // TODO: test with a huge file
        }

        grpc::Status const status{ reader->Finish() };
        if ( !status.ok() )
        {
            spdlog::error( "Error while downloading the file '{}', error: {}", file, status.error_message() );
            return false;
        }

        auto * rawdata{ data.release_rawdata() };

        WAV::File wavFile
        {
            parseMetadata( metadata ),
            reinterpret_cast< std::byte * >( rawdata->data() ), // takes ownership
            static_cast< std::uint32_t >( rawdata->size() )
        };
        // wavFile.riff.size = metadata.filesize();

        if ( !wavFile.valid() )
        {
            spdlog::error( "Received file is not valid!" );
            return false;
        }

        if ( !wavFile.write( output_path ) )
        {
            spdlog::error( "Writing file to file failed" );
            return false;
        }
        // else
        // {
            // spdlog::info( "Written {} bytes to '{}'", wavFile.size_in_bytes(), output_path );
        // }
        return true;
    }
} // namespace Teleaudio

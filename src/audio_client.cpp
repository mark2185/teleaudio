#include "audio_client.hpp"

#include <spdlog/spdlog.h>

#include "audio_server.hpp"
#include "wav.hpp"

namespace
{
    WAV::FmtSubChunk parseMetadata( Teleaudio::AudioMetadata const metadata )
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

        // reading the raw audio data
        while ( reader->Read( &data ) )
        {
            // TODO: test with a huge file
        }

        grpc::Status const status{ reader->Finish() };
        if ( !status.ok() )
        {
            spdlog::error( "Error while downloading the file '{}', error: {}", filename, status.error_message() );
            return std::nullopt;
        }

        auto * rawdata{ data.release_rawdata() };

        WAV::File file
        {
            parseMetadata( metadata ),
            reinterpret_cast< std::byte * >( rawdata->data() ), // takes ownership
            static_cast< std::uint32_t >( rawdata->size() )
        };
        // wavFile.riff.size = metadata.filesize();

        if ( !file.valid() )
        {
            spdlog::error( "Received file is not valid!" );
            return std::nullopt;
        }

        return file;
    }


    bool AudioClient::Play( std::string_view const file ) const
    {
        auto const wav_file{ receiveFile( file ) };
        if ( !wav_file.has_value() )
        {
            return false;
        }
        // TODO: actually play the file
        return true;
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
            spdlog::error( "Writing file to file failed" );
            return false;
        }
        return true;
    }
} // namespace Teleaudio

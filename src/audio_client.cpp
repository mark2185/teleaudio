#include "audio_client.hpp"

#include <spdlog/spdlog.h>
#include "wav.hpp"
#include "audio_server.hpp"

namespace Teleaudio
{
    // TODO: cast operator?
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

    // std::string AudioClient::RunCmd( std::string const & cmd, std::string const & arg )
    // {
        // using namespace audioservice;

        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        // ClientContext context;

        // if ( cmd == "play" )
        // {
            // // Data we are sending to the server.
            // File request;
            // request.set_name(arg);
            // std::unique_ptr<ClientReader<AudioData> > reader{ stub_->Play( &context, request ) };

            // spdlog::info( "Reading data back" );
            // AudioData data;
            // reader->Read( &data );
            // spdlog::info( "Metadata: {}, {}, {}", 
                    // data.metadata().averagebytespersecond(),
                    // data.metadata().bitspersample(),
                    // data.metadata().channels());

            // auto const metadata{ data.metadata() };

            // spdlog::info( "Starting read loop" );
            // while( reader->Read( &data ) )
            // {
                // spdlog::info( "Read some raw data!" );
                // spdlog::info( "{}", data.rawdata().size() );
            // }
            // spdlog::info( "End of reading loop" );

            // Status status = reader->Finish();
            // if ( status.ok() )
            // {
                // auto wavFile{ WAV::constructPlaceholderWaveFile( parseMetadata( metadata ), reinterpret_cast< std::byte * >( const_cast< char * >( data.rawdata().data() ) ), data.rawdata().size() ) };
                // wavFile.riff.size = metadata.filesize();

                // // spdlog::info( "riff size {}, chunk1 size {}, chunk2 size: {}", wavFile.riff.size, wavFile.format.subchunk1_size, wavFile.data.subchunk2_size );
                // spdlog::info( "File size in bytes: {}", wavFile.size_in_bytes() );
                // if ( !wavFile.write( "/home/mark/desktop/asdf.wav" ) )
                // {
                    // spdlog::error( "Writing file to file failed" );
                // }
                // return "status is ok";
            // }
            // else
            // {
                // spdlog::error( "playing failed, nothing to write" );
                // return "playing failed";
            // }
        // }
        // else
        // {
            // // Data we are sending to the server.
            // Command request;
            // request.set_cmd(cmd);
            // request.set_arg(arg);

            // // Container for the data we expect from the server.
            // CmdOutput reply;

            // // The actual RPC.
            // Status status = stub_->RunCmd(&context, request, &reply);

            // // Act upon its status.
            // if (status.ok()) {
                // return reply.message();
            // } else {
                // spdlog::error( "RPC failed: {}", status.error_message().c_str() );
                // return "RPC failed";
            // }
        // }
    // }

    std::string AudioClient::List( std::string_view const directory ) const
    {
        grpc::ClientContext context;

        Directory request;
        request.set_path( directory.data() );

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

        // auto const channels{ data.metadata().channels() };
        spdlog::info( "Playing: {} (bps: {}, channels: {}, size: {}", file, 1, 2, 3 ); // TODO

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

        std::unique_ptr< grpc::ClientReader< AudioData > > reader{ stub_->Play( &context, request ) };

        // reading metadata first
        AudioData data;
        reader->Read( &data );

        // auto const channels{ data.metadata().channels() };
        spdlog::info( "Playing: {} (bps: {}, channels: {}, size: {}", file, 1, 2, 3 ); // TODO take MPV as inspiration for the necessary data

        auto const metadata{ data.metadata() };

        // reading the raw audio data
        while ( reader->Read( &data ) )
        {
            // TODO: test with a huge file
        }

        grpc::Status const status{ reader->Finish() };
        if ( !status.ok() )
        {
            spdlog::error( "Error while downloading the file '{}', error: {}", file, status.error_message() );
            return false;
        }

        auto wavFile
        {
            WAV::constructPlaceholderWaveFile
            (
                parseMetadata( metadata ),
                reinterpret_cast< std::byte * >( const_cast< char * >( data.rawdata().data() ) ),
                data.rawdata().size()
            )
        };
        wavFile.riff.size = metadata.filesize();

        if ( !wavFile.write( output_path ) )
        {
            spdlog::error( "Writing file to file failed" );
            return false;
        }
        return true;
    }
} // namespace Teleaudio

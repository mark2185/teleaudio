#include "audio_server.hpp"

#include <spdlog/spdlog.h>
#include "wav.hpp"

namespace audioserver
{
    WAV::FmtSubChunk parseMetadata( audioservice::AudioMetadata const metadata )
    {
        return
        {
            .subchunk1_size  = 16, // fixed
            .audio_format    = 1,  // PCM
            .num_channels    = static_cast< std::uint16_t >( metadata.channels() ),
            .sample_rate     =                             ( metadata.samplerate() ),
            .byte_rate       =                             ( metadata.averagebytespersecond() ),
            .block_align     = static_cast< std::uint16_t >( metadata.blockalign() ),
            .bits_per_sample = static_cast< std::uint16_t >( metadata.bitspersample() ),
        };
    }

    using namespace audioservice;
    std::string AudioClient::RunCmd( std::string const & cmd, std::string const & arg )
    {
        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        if ( cmd == "play" )
        {
            // Data we are sending to the server.
            File request;
            request.set_name(arg);
            std::unique_ptr<ClientReader<AudioData> > reader{ stub_->Play( &context, request ) };
            spdlog::info( "Played audio" );

            spdlog::info( "Reading data back" );
            AudioData data;
            reader->Read( &data );
            spdlog::info( "Metadata: {}, {}, {}", 
                    data.metadata().averagebytespersecond(),
                    data.metadata().bitspersample(),
                    data.metadata().channels());

            auto const metadata{ data.metadata() };

            spdlog::info( "Starting read loop" );
            while( reader->Read( &data ) )
            {
                spdlog::info( "Read some raw data!" );
                spdlog::info( "{}", data.rawdata().size() );
            }
            spdlog::info( "End of reading loop" );

            Status status = reader->Finish();
            if ( status.ok() )
            {
                auto wavFile{ WAV::constructPlaceholderWaveFile( parseMetadata( metadata ), reinterpret_cast< std::byte * >( const_cast< char * >( data.rawdata().data() ) ), data.rawdata().size() ) };
                // wavFile.format = parseMetadata( metadata );
                // spdlog::info( "Metadata: {}", data.mutable_meta()->averagebytespersecond() );
                auto const output_file{ FileUtils::openFile( "/tmp/asdf.wav", FileUtils::FileOpenMode::WriteBinary ) };
                if ( !output_file )
                {
                    spdlog::error( "Cannot open output file" );
                }
                else
                {
                    auto const res{ std::fwrite( &wavFile, 1, wavFile.size_in_bytes(), output_file.get() ) };
                    if ( res != wavFile.size_in_bytes() )
                    {
                        spdlog::info( "Couldn't write to file, wrote only {} bytes", res );
                    }
                    else
                    {
                        spdlog::info( "Successfully written {} bytes to /tmp/asdf.wav", wavFile.size_in_bytes() );
                    }
                }
                return "status is ok";
            }
            else
            {
                spdlog::error( "playing failed, nothing to write" );
                return "playing failed";
            }
        }
        else
        {
            // Data we are sending to the server.
            Command request;
            request.set_cmd(cmd);
            request.set_arg(arg);

            // Container for the data we expect from the server.
            CmdOutput reply;

            // The actual RPC.
            Status status = stub_->RunCmd(&context, request, &reply);

            // Act upon its status.
            if (status.ok()) {
                return reply.message();
            } else {
                spdlog::error( "RPC failed: {}", status.error_message().c_str() );
                return "RPC failed";
            }
        }
    }

} // namespace audioserver

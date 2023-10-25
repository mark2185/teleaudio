#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "communication.grpc.pb.h"

#include <spdlog/spdlog.h>
#include "audio_server.hpp"
#include "wav.hpp"

#include <filesystem>

namespace fs = std::filesystem;

static fs::path storage_directory;

namespace
{
    [[ nodiscard ]] std::string ls( std::string_view const directory )
    {
        std::stringstream ss;
        spdlog::debug( "Looking for all files in the directory {}", ( storage_directory / directory ).string() );
        for ( auto const & entry : fs::directory_iterator( storage_directory / directory ) ) 
        {
            if ( entry.is_regular_file() && entry.path().extension() == ".wav" )
            {
                ss << entry.path().filename() << '\n';
            }
        }
        return ss.str();
    }
}

namespace audioservice
{
    using namespace audioserver;
    class AudioServiceImpl final : public AudioService::Service {

        Status RunCmd( ServerContext* context, Command const * request, CmdOutput * output ) override
        {
            constexpr std::array const valid_commands{ "ls", "play" };
            auto const cmd{ request->cmd() };
            if ( std::find( std::begin( valid_commands ), std::end( valid_commands ), request->cmd() ) == std::end( valid_commands ) )
            {
                output->set_message( "command not supported\n" );
            }
            else if ( request->cmd() == "ls" )
            {
                auto const dir{ request->arg() };
                output->set_message( ls( dir ) );
            }
            else
            {
                // TODO: play a sound
                // TODO: this is done from the client side, maybe we should have listed commands in the .proto service definition
            }
            return Status::OK;
        }

        AudioMetadata setMetadata( WAV::FmtSubChunk const fmt )
        {
            AudioMetadata ret;
            ret.set_averagebytespersecond( fmt.byte_rate );
            ret.set_bitspersample        ( fmt.bits_per_sample );
            ret.set_blockalign           ( fmt.block_align );
            ret.set_channels             ( fmt.num_channels );
            // ret.set_extrasize            ( fmt.);
            ret.set_samplerate           ( fmt.sample_rate);

            return ret;
        }

        Status Play( ServerContext* context, File const * request, ServerWriter< AudioData > * writer ) override
        {
            auto const file{ storage_directory / request->name() };
            if ( !fs::exists( file ) )
            {
                spdlog::error( "File '{}' not available for playing.", file.string() );
                return Status::OK;
            }

            spdlog::debug( "Playing file {}", file.string() );

            WAV::File const song{ file.string() };
            if ( !song.valid() )
            {
                // TODO: there are 40 bytes extra in the file AWESOME.wav somewhere
                // the math doesn't add up
                spdlog::debug( "Loaded file is not valid" );
            }

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

            // sending metadata first
            AudioMetadata metadata{ setMetadata( song.format ) };
            // TODO: maybe a cast operator
            metadata.set_filesize( song.size_in_bytes() - 8 );
            // spdlog::info( "Writing size to metadata: {}", song.size_in_bytes() - 8 );

            AudioData response;
            *response.mutable_metadata() = metadata;
            if ( !writer->Write( response ) )
            {
                spdlog::error( "Sending metadata failed, returning" );
                return Status::OK;
            }

            auto const rawData{ song.data.data };
            auto const rawDataSize{ song.data.subchunk2_size };
            spdlog::debug( "Raw data size: {}", rawDataSize );

            // streaming the raw samples
            AudioData payload;
            payload.set_rawdata( reinterpret_cast< char const * >( rawData ), rawDataSize );

            // spdlog::info( "Sending {} bytes, but actual filesize is {}", payload.ByteSizeLong(), song.size_in_bytes() );
            if ( !writer->Write( payload ) )
            {
                spdlog::error( "Failed to write raw data." );
            }
            else
            {
                spdlog::info( "Successfully sent raw data!" );
            }
            return Status::OK;
        }
    };

} // namespace audioservice

namespace audioserver
{
    void run_server( std::string_view const directory, std::int16_t const port )
    {
        storage_directory = directory;
        // TODO: append given port to the address
        std::string const server_address{ "0.0.0.0:5371" };

        audioservice::AudioServiceImpl service;

        // grpc::EnableDefaultHealthCheckService(true);
        // grpc::reflection::InitProtoReflectionServerBuilderPlugin();
        ServerBuilder builder;

        // Listen on the given address without any authentication mechanism.
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        // Register "service" as the instance through which we'll communicate with
        // clients. In this case it corresponds to a *synchronous* service.
        builder.RegisterService(&service);
        // Finally assemble the server.
        std::unique_ptr<Server> server(builder.BuildAndStart());

        spdlog::info( "Server listening on {}", server_address );

        // Wait for the server to shutdown. Note that some other thread must be
        // responsible for shutting down the server for this call to ever return.
        server->Wait();
    }
}

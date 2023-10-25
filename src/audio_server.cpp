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
        spdlog::info( "Looking for all files in the directory {}", ( storage_directory / directory ).string() );
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
                // play a sound
            }
            return Status::OK;
        }

        Status Play( ServerContext* context, File const * request, ServerWriter< AudioData > * writer ) override
        {
            auto const file{ storage_directory / request->name() };
            if ( !fs::exists( file ) )
            {
                spdlog::error( "File '{}' not available for playing", file.string() );
                return Status::OK;
            }
            spdlog::info( "Playing file {}", file.string() );
            WAV::File const song{ file.string() };
            if ( !song.valid() )
            {
                if ( !song.riff.valid() )
                {
                    spdlog::error( "RIFF not valid!" );
                }
                if ( !song.format.valid() )
                {
                    spdlog::error( "Format not valid!" );
                }
                if ( !song.data.valid() )
                {
                    spdlog::error( "Data not valid!" );
                }
                spdlog::error( "Failed to open song '{}'", file.string() );
                return Status::OK;
            }

            auto const [ id, size, format, n, sample, byte, align, bps ]{ song.format };
            // spdlog::info( "Fmt: size = {}, format = {}, numChannels = {}, sample_rate = {}, byte_rate = {}, block_align = {}, bps = {}", size, format, n, sample, byte, align, bps );
            // spdlog::info( "Parsing successful!" );

            // spdlog::info( "Just returning from play" );
            // AudioMetadata meta;

            AudioData response{};
            // response.mutable_audiometadata()->set_averagebytespersecond( byte );
            response.mutable_metadata()->set_averagebytespersecond( byte );
            // ->set_averagebytespersecond( song.format.byte_rate );
            // spdlog::info( "Set averagebytespersecond" );
            // {
                // .AverageBytesPerSecond = 1,
                // .BitsPerSample = 2,
                // .BlockAlign = 3,
                // .Channels = 4,
                // .ExtraSize = 5,
                // .SampleRate = 6,
            // };
            // writer->Write( AudioData
            if ( writer->Write( response ) )
            {
                spdlog::info( "Write was successful, returning" );
            }
            else
            {
                spdlog::error( "Failed to write metadata" );
            }
            return Status::OK;
        }
    };
} // namespace audioservice

namespace audioserver
{
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
            AudioData data{};
            while( reader->Read( &data ) )
            {
                spdlog::info( "Read some data!" );
                spdlog::info( "Metadata: {}", data.metadata().averagebytespersecond() );
            }
            spdlog::info( "End of reading loop" );
            Status status = reader->Finish();
            if ( status.ok() )
            {
                // spdlog::info( "Metadata: {}", data.mutable_meta()->averagebytespersecond() );
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

    void run_server( std::string_view const directory, std::int16_t const port )
    {
        storage_directory = directory;
        // TODO:
        std::string const server_address{ "0.0.0.0:5371" };

        AudioServiceImpl service;

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


} // namespace audioserver

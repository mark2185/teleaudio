#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "communication.grpc.pb.h"

#include <spdlog/spdlog.h>
#include "audio_server.hpp"

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
        Status RunCmd( ServerContext* context, Command const * request, CmdOutput * output ) override {
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

    };
} // namespace audioservice

namespace audioserver
{
    using namespace audioservice;
    std::string AudioClient::RunCmd( std::string const & cmd, std::string const & arg )
    {
        // Data we are sending to the server.
        Command request;
        request.set_cmd(cmd);
        request.set_arg(arg);

        // Container for the data we expect from the server.
        CmdOutput reply;

        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        // The actual RPC.
        Status status = stub_->RunCmd(&context, request, &reply);

        // Act upon its status.
        if (status.ok()) {
            return reply.message();
        } else {
            // TODO:
            // spdlog::error( "{}: {}", status.error_code(), status.error_message() );
            return "RPC failed";
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

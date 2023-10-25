#pragma once
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include "communication.grpc.pb.h"

namespace audioserver
{
    using grpc::Channel;
    using grpc::ClientContext;
    using grpc::Server;
    using grpc::ClientReader;
    using grpc::ServerWriter;
    using grpc::Status;
    using grpc::ServerContext;
    using grpc::ServerBuilder;

    void run_server( std::string_view directory, std::int16_t port);

    class AudioClient {
        public:
            AudioClient(std::shared_ptr<Channel> channel)
                : stub_(audioservice::AudioService::NewStub(channel)) {}

            // Assembles the client's payload, sends it and presents the response back
            // from the server.
            std::string RunCmd( std::string const & cmd, std::string const & arg );

            // Assembles the client's payload, sends it and presents the response back
            // from the server.
            // std::string Play( std::string const & cmd, std::string const & arg );

        private:
            std::unique_ptr<audioservice::AudioService::Stub> stub_;
    };
}

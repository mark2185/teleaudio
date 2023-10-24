#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "communication.grpc.pb.h"

using grpc::Server;
using grpc::Status;
using grpc::ServerContext;

namespace audioservice
{
    class AudioServiceImpl final : public AudioService::Service {
        // Status RunCmd( ServerContext* context, Command const * request, CmdOutput * output ) override {

            // return Status::OK;
        // }

    };
}

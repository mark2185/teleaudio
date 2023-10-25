#pragma once

#include <memory>
#include <grpcpp/grpcpp.h>

#include "communication.grpc.pb.h"

namespace Teleaudio
{

class AudioClient
{
public:
    AudioClient( std::shared_ptr< grpc::Channel > channel )
        : stub_{ AudioService::NewStub( channel ) }
    {}

    // Takes an optional directory as an argument
    [[ nodiscard ]] std::string List     ( std::string_view directory = "." ) const;

    [[ nodiscard ]] bool Play     ( std::string_view file                               ) const;
    [[ nodiscard ]] bool Download ( std::string_view file, std::string_view output_path ) const;

private:
    std::unique_ptr< Teleaudio::AudioService::Stub > stub_;
};

} // namespace Teleaudio

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

    // Print out the contents of a directory
    [[ nodiscard ]] std::string List     ( std::string_view directory = "." ) const;

    // Play the file on an audio device
    [[ nodiscard ]] bool Play     ( std::string_view file                               ) const;

    // Download the file
    [[ nodiscard ]] bool Download ( std::string_view file, std::string_view output_path ) const;

private:
    std::unique_ptr< Teleaudio::AudioService::Stub > stub_;
};

} // namespace Teleaudio

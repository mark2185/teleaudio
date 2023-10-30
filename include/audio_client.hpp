#pragma once

#include <memory>
#include <grpcpp/grpcpp.h>
#include <optional>

#include "communication.grpc.pb.h"

// fwd
namespace WAV { struct File; };

namespace Teleaudio
{

class AudioClient
{
public:
    AudioClient( std::shared_ptr< grpc::Channel > channel )
        : stub_{ AudioService::NewStub( channel ) }
    {}

    // Returns the contents of a directory
    [[ nodiscard ]] std::string List( std::string_view directory = "." ) const;

    // Play the file on an audio device
    [[ nodiscard ]] bool Play( std::string_view file ) const;

    // Download the file and write it to given 'output_path'
    [[ nodiscard ]] bool Download ( std::string_view file, std::string_view output_path ) const;

private:
    // helper for making a connection and receiving the file
    [[ nodiscard ]] std::optional< WAV::File > receiveFile( std::string_view file ) const;

    std::unique_ptr< Teleaudio::AudioService::Stub > stub_;
};

} // namespace Teleaudio

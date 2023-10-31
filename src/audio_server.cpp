
#include "audio_server.hpp"
#include "communication.grpc.pb.h"
#include "wav.hpp"

#include <cstdint>
#include <filesystem>
#include <grpcpp/grpcpp.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>

namespace fs = std::filesystem;

// TODO: chroot jail
// the directory where the audio files are
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

    [[ nodiscard ]] Teleaudio::AudioMetadata setMetadata( WAV::FmtSubChunk const fmt )
    {
        Teleaudio::AudioMetadata ret;
        ret.set_averagebytespersecond( fmt.byte_rate       );
        ret.set_bitspersample        ( fmt.bits_per_sample );
        ret.set_blockalign           ( fmt.block_align     );
        ret.set_channels             ( fmt.num_channels    );
        ret.set_samplerate           ( fmt.sample_rate     );

        return ret;
    }
}

namespace Teleaudio
{

class TeleaudioImpl final : public AudioService::Service
{

    grpc::Status List( grpc::ServerContext *, Directory const * request, CmdOutput * response ) override
    {
        response->set_text( ls( request->path() ) );
        return grpc::Status::OK;
    }

    grpc::Status Download( grpc::ServerContext *, File const * request, grpc::ServerWriter< AudioData > * writer ) override
    {
        auto const file{ storage_directory / request->name() };
        if ( !fs::exists( file ) )
        {
            spdlog::error( "File '{}' not available for playing.", file.string() );
            return grpc::Status::OK;
        }

        auto song{ WAV::File::load( file.string() ) };
        if ( !song )
        {
            spdlog::debug( "Aborting because file is not valid" );
            return grpc::Status::OK;
        }

        // sending metadata first
        AudioMetadata metadata{ setMetadata( song->format ) };
        metadata.set_rawdatasize( song->data.size );

        AudioData metadata_response;
        *metadata_response.mutable_metadata() = metadata;

        if ( !writer->Write( metadata_response ) )
        {
            spdlog::error( "Sending metadata failed, exiting" );
            return grpc::Status::OK;
        }

        // sending the raw data, chunking if bigger than `chunk_size`

        std::uint32_t const chunk_size     { 5 * 1024 }; // 5 KiB
        std::uint32_t       bytes_remaining{ song->data.size };
        std::uint32_t       byte_offset    {};

        auto const payload_data{ reinterpret_cast< char const * >( song->data.data ) };

        AudioData rawdata_response;
        while ( bytes_remaining > 0 )
        {
            auto const payload_size{ std::min( bytes_remaining, chunk_size ) };

            rawdata_response.set_rawdata( payload_data + byte_offset, payload_size );

            if ( !writer->Write( rawdata_response ) )
            {
                spdlog::error( "Failed to write raw data." );
                return grpc::Status::CANCELLED;
            }

            bytes_remaining -= payload_size;
            byte_offset     += payload_size;
        }

        spdlog::info( "Sent {}/{} bytes in total", byte_offset, song->data.size );

        return grpc::Status::OK;
    }

}; // class TeleaudioImpl


void run_server( std::string_view const directory, std::uint16_t const port )
{
    storage_directory = directory;

    auto const server_address{ "0.0.0.0:" + std::to_string( port ) };

    Teleaudio::TeleaudioImpl service;

    grpc::ServerBuilder builder;

    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

    // Register "service" as the instance through which we'll synchronously
    // communicate with clients.
    builder.RegisterService(&service);

    // Finally assemble and start the server.
    std::unique_ptr< grpc::Server > server( builder.BuildAndStart() );

    spdlog::info( "Server listening on {}", server_address );

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    // TODO: shut the server down in tests
    server->Wait();
}

} // namespace Teleaudio

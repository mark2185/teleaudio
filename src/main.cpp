#include <cstdio>

#include "audio_client.hpp"
#include "audio_server.hpp"
#include "wav.hpp"
#include "spdlog/spdlog.h"
#include <charconv>


void print_help()
{
    spdlog::error( "\nUsage:\n\t$> ./teleaudio server /path/to/wav/files <port>\nOr:\n\t$> ./teleaudio" );
}

int run_client( int argc, char const * argv [] )
{
    run_client( argc, argv );
    Teleaudio::AudioClient c{ grpc::CreateChannel("localhost:5371", grpc::InsecureChannelCredentials()) };

    spdlog::info( "ls\n{}", c.List() );

    // spdlog::info( "play AMAZING_clean.wav" );
    // if ( !c.Play( "AMAZING_clean.wav" ) )
    // {
        // spdlog::error( "Something went wrong with cmd 'play AMAZING.wav'" );
    // }
    spdlog::info( "download AMAZING_clean.wav /tmp/amazing.wav" );
    if ( !c.Download( "AMAZING_clean.wav", "/tmp/amazing.wav" ) )
    {
        spdlog::error( "Something went wrong with cmd 'download AMAZING.wav /tmp/amazing.wav'" );
    }
    spdlog::info( "Exiting!" );
    return 0;
}

int run_server( int argc, char const * argv [] )
{
    if ( argc != 4 || argv[ 1 ] != std::string{ "server" } )
    {
        spdlog::error( "Wrong number of parameters!" );
        print_help();
        return 1;
    }

    auto const storage{ argv[ 2 ] };
    std::string const port_arg{ argv[ 3 ] };

    int port;
    std::from_chars( port_arg.data(), port_arg.data() + port_arg.size(), port );
    Teleaudio::run_server( storage, port );

    return 0;
}

int main( int argc, char const * argv[] )
{
    if ( argc == 1 )
    {
        return run_client( argc, argv );
    }
    else
    {
        return run_server( argc, argv );
    }
    return 0;
}

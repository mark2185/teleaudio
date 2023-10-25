#include <cstdio>

#include "audio_server.hpp"
#include "wav.hpp"
#include "spdlog/spdlog.h"
#include <charconv>


void print_help()
{
    spdlog::error( "\nUsage:\n\t$> ./teleaudio server <port>\nOr:\n\t$> ./teleaudio <command> [arg]" );
}

int main( int argc, char const * argv[] )
{
    if ( argc > 1 &&  std::string{ "server" } == argv[ 1 ] )
    {
        if ( argc != 3 )
        {
            spdlog::error( "Wrong number of parameters!" );
            print_help();
            return 1;
        }
        std::string const port_arg{ argv[ 2 ] };
        int port;
        std::from_chars( port_arg.data(), port_arg.data() + port_arg.size(), port );
        audioserver::run_server( port );
    }
    else
    {
        audioserver::AudioClient c{ grpc::CreateChannel("localhost:5371", grpc::InsecureChannelCredentials()) };
        std::string const cmd{ "ls" };
        std::string const reply{ c.RunCmd( cmd, "" ) };
        spdlog::info( "Received: {}", reply );
    }
    // WAV::File song{ "/home/mark/workspace/gits/teleaudio/test/storage/Speech/Swedish/GOAWAY.WAV" };
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
        // spdlog::error( "Given song is not valid!" );
        // return 1;
    // }

    // auto const [ id, size, format, n, sample, byte, align, bps ]{ song.format };
    // spdlog::info( "Fmt: size = {}, format = {}, numChannels = {}, sample_rate = {}, byte_rate = {}, block_align = {}, bps = {}", size, format, n, sample, byte, align, bps );
    // spdlog::info( "Parsing successful!" );
    return 0;
}

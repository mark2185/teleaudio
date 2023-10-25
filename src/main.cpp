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

int main( int argc, char const * argv[] )
{
    if ( argc == 1 )
    {
        Teleaudio::AudioClient c{ grpc::CreateChannel("localhost:5371", grpc::InsecureChannelCredentials()) };
        // std::string const cmd{ "ls" };
        // std::string const reply{ c.RunCmd( cmd, "" ) };
        // spdlog::info( "Received:\n{}", c.RunCmd( "ls", "" ) );
        spdlog::info( "Received:\n{}", c.List() );
        spdlog::info( "Received:\n{}", c.Play( "AMAZING.wav" ) );
        spdlog::info( "Received:\n{}", c.Download( "AMAZING.wav", "/tmp/amazing.wav" ) );
        spdlog::info( "Exiting!" );
    }
    else if ( std::string{ "server" } == argv[ 1 ] )
    {
        if ( argc != 4 )
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

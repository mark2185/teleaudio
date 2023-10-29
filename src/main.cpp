#include <charconv>
#include <cstdio>
#include <filesystem>

#include "audio_client.hpp"
#include "audio_server.hpp"
#include "wav.hpp"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace fs = std::filesystem;

std::shared_ptr< spdlog::logger > logger{};

void print_help()
{
    spdlog::error( "\nUsage:\n\t$> ./teleaudio server /path/to/wav/files <port>\nOr:\n\t$> ./teleaudio <port> <destination-folder>" );
}

int run_client( char const * argv [] )
{
    std::string const port_arg{ argv[ 1 ] };
    int port{};
    std::from_chars( port_arg.data(), port_arg.data() + port_arg.size(), port );

    Teleaudio::AudioClient c{ grpc::CreateChannel("localhost:" + std::to_string( port ), grpc::InsecureChannelCredentials()) };

    auto const output_directory{ argv[ 2 ] };

    spdlog::info( "ls\n{}", c.List() );

    // download small file
    {
        auto const song_name{ "AMAZING_clean.wav" };
        spdlog::info( "download {} {}/{}", song_name, output_directory, song_name );
        auto const output_path{ fs::path{ output_directory } / song_name };
        if ( !c.Download( song_name, output_path.string() ) )
        {
            spdlog::error( "Something went wrong with cmd 'download {} {}'", song_name, output_path.string() );
        }
    }

    // download bigger file
    {
        auto const song_name{ "big_file.wav" };
        spdlog::info( "download {} {}/{}", song_name, output_directory, song_name );
        auto const output_path{ fs::path{ output_directory } / song_name };
        if ( !c.Download( song_name, output_path.string() ) )
        {
            spdlog::error( "Something went wrong with cmd 'download {} {}'", song_name, output_path.string() );
        }
    }

    // play a song
    {
        auto const song_name{ "big_file.wav" };

        spdlog::info( "play {}", song_name );
        if ( !c.Play( song_name ) )
        {
            spdlog::error( "Something went wrong with cmd 'play {}'", song_name );
        }
    }

    spdlog::info( "Exiting!" );
    return 0;
}

int run_server( char const * argv [] )
{
    if ( argv[ 1 ] != std::string{ "server" } )
    {
        spdlog::error( "Wrong number of parameters!" );
        print_help();
        return 1;
    }

    std::string const port_arg{ argv[ 2 ] };
    auto const storage{ argv[ 3 ] };

    int port{};
    std::from_chars( port_arg.data(), port_arg.data() + port_arg.size(), port );
    Teleaudio::run_server( storage, static_cast< std::uint16_t >( port ) );

    return 0;
}

void create_logger_with_multiple_sinks()
{
    auto const logfile{ fs::temp_directory_path() / "teleaudio.log" };

    auto console_sink{ std::make_shared< spdlog::sinks::stdout_sink_st >() };
    auto file_sink   { std::make_shared< spdlog::sinks::basic_file_sink_mt >( logfile.string() ) };

    std::array< spdlog::sink_ptr, 2 >  const sinks
    {
        console_sink, file_sink
    };

    auto const combined_logger{ std::make_shared< spdlog::logger >( "combined_logger", std::begin(sinks), std::end( sinks ) ) };

    spdlog::register_logger( combined_logger );

    spdlog::set_default_logger( combined_logger );

    logger = spdlog::get( "combined_logger" );

    logger->flush_on(spdlog::level::debug);

    spdlog::info( "Logging onto stdout, but also {}.", logfile.string() );
}

int main( int argc, char const * argv[] )
{
    create_logger_with_multiple_sinks();

    //  client
    if ( argc == 3 )
    {
        return run_client( argv );
    }
    // server
    else if ( argc == 4 )
    {
        return run_server( argv );
    }

    print_help();

    return 0;
}

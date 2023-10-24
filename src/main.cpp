#include <cstdio>

#include "wav.hpp"
#include "spdlog/spdlog.h"

int main( int argc, char const * argv[] )
{

    WAV::File song{ "/home/mark/workspace/gits/teleaudio/test/storage/Speech/Swedish/GOAWAY.WAV" };
    if ( !song.valid() )
    {
        if ( !song.riff.valid() )
        {
            spdlog::error( "RIFF not valid!" );
        }
        if ( !song.format.valid() )
        {
            spdlog::error( "Format not valid!" );
        }
        if ( !song.data.valid() )
        {
            spdlog::error( "Data not valid!" );
        }
        spdlog::error( "Given song is not valid!" );
        return 1;
    }

    auto const [ id, size, format, n, sample, byte, align, bps ]{ song.format };
    spdlog::info( "Fmt: size = {}, format = {}, numChannels = {}, sample_rate = {}, byte_rate = {}, block_align = {}, bps = {}", size, format, n, sample, byte, align, bps );
    spdlog::info( "Parsing successful!" );
    return 0;
}

#pragma once

#include <string_view>
#include <cstdint>

namespace Teleaudio
{
    void run_server( std::string_view directory, std::uint16_t port );
}

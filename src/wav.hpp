#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include "spdlog/spdlog.h"

#include "utils.hpp"

namespace WAV
{
struct MagicBytes
{
    static constexpr std::array< std::byte, 4 > RIFF = { std::byte{ 0x52 }, std::byte{ 0x49 }, std::byte{ 0x46 }, std::byte{ 0x46 } };
    static constexpr std::array< std::byte, 4 > WAVE = { std::byte{ 0x57 }, std::byte{ 0x41 }, std::byte{ 0x56 }, std::byte{ 0x45 } };
    static constexpr std::array< std::byte, 4 > fmt  = { std::byte{ 0x66 }, std::byte{ 0x6d }, std::byte{ 0x74 }, std::byte{ 0x20 } };
    static constexpr std::array< std::byte, 4 > data = { std::byte{ 0x64 }, std::byte{ 0x61 }, std::byte{ 0x74 }, std::byte{ 0x61 } };
};


struct RiffChunk
{
    std::array< std::byte, 4 > id;
    std::uint32_t              size;
    std::array< std::byte, 4 > format;

    [[ nodiscard ]] bool valid() const
    {
        auto const magic_bytes_match
        {
            std::memcmp(     id.data(), MagicBytes::RIFF.data(), MagicBytes::RIFF.size() ) == 0
         && std::memcmp( format.data(), MagicBytes::WAVE.data(), MagicBytes::WAVE.size() ) == 0
        };
        return magic_bytes_match;
    }
} __attribute__((packed));

struct FmtSubChunk
{
    std::array< std::byte, 4 > subchunk1_id;
    std::uint32_t subchunk1_size;
    std::uint16_t audio_format;
    std::uint16_t num_channels;
    std::uint32_t sample_rate;
    std::uint32_t byte_rate;
    std::uint16_t block_align;
    std::uint16_t bits_per_sample;

    [[ nodiscard ]] bool valid() const
    {
        auto const magic_bytes_match{ std::memcmp( subchunk1_id.data(), MagicBytes::fmt.data(), MagicBytes::fmt.size() ) == 0 };
        auto const expected_byte_rate{ sample_rate * num_channels * bits_per_sample / 8 };
        auto const expected_block_align{ num_channels * bits_per_sample / 8 };
        return magic_bytes_match
            && expected_byte_rate   == byte_rate
            && expected_block_align == block_align;
    }
} __attribute__((packed));

struct DataSubchunk
{
    std::array< std::byte, 4 > subchunk2_id;
    std::uint32_t subchunk2_size;
    std::byte * data;

    [[ nodiscard ]] bool valid() const
    {
        auto const magic_bytes_match{ std::memcmp( subchunk2_id.data(), MagicBytes::data.data(), MagicBytes::data.size() ) == 0 };
        return magic_bytes_match;
    }

} __attribute__((packed));

struct File
{
    RiffChunk riff{};
    FmtSubChunk format{};
    DataSubchunk data{};

    // FileUtils::FilePtr file_handle;

    [[ nodiscard ]] bool valid() const
    {
        auto const riffValid{ riff.valid() };
        auto const formatValid{ format.valid() };
        auto const dataValid{ data.valid() };
        return riff.valid() && format.valid() && data.valid();
    }


    // TODO: rule of five
    // TODO: deep copy
    // ~File() = default;
    // File          ( File const & ) = delete;
    // File operator=( File const & ) = delete;

    File( std::string_view const filename )
    {
        auto file_handle{  FileUtils::openFile( filename, FileUtils::FileOpenMode::ReadBinary ) };
        if ( !file_handle )
        {
            return;
        }

        auto buffer{ std::make_unique< std::byte[] >( sizeof( WAV::File ) ) };
        std::fread( buffer.get(), sizeof( WAV::File ), 1, file_handle.get() );

        *this = *reinterpret_cast< WAV::File * >( buffer.get() );
    }

} __attribute__((packed)); // TODO: msvc

} // namespace WAV

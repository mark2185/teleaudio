#include <cstdio>
#include <memory>
#include <cstdint>
#include <string_view>

namespace FileUtils
{
    using FilePtr = std::unique_ptr< FILE, decltype( &std::fclose ) >;

    enum class FileOpenMode : std::uint8_t
    {
        ReadText,
        ReadBinary,
        WriteText,
        WriteBinary
    };

    [[ nodiscard ]] inline constexpr auto fileOpenModeToString( FileOpenMode const fom )
    {
        switch( fom )
        {
            case FileOpenMode::ReadText:    return "r";
            case FileOpenMode::ReadBinary:  return "rb";
            case FileOpenMode::WriteText:   return "w";
            case FileOpenMode::WriteBinary: return "wb";
            default:                        return "r";
        }
    }

    [[ nodiscard ]] inline FilePtr openFile( std::string_view const path, FileOpenMode const fom )
    {
        return
        {
            std::fopen( path.data(), fileOpenModeToString( fom ) ),
            &std::fclose
        };
    }
} // namespace Utils

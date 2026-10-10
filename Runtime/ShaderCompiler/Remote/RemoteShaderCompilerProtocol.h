#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Json/JsonValue.h"
#include "ShaderCore/ShaderTypes.h"

struct FShaderCompilerIdentity;

struct SHADERCOMPILER_API RemoteShaderCompilerProtocol
{
    static constexpr uint16 DefaultPort    = 9371;
    static constexpr uint32 Version        = 2;
    static constexpr uint32 Magic          = 0x53525844; // 'DXRS'
    static constexpr uint32 MaxHeaderSize  = 1 * 1024 * 1024;
    static constexpr uint32 MaxPayloadSize = 64 * 1024 * 1024;
    static constexpr int32  MaxForwardHops = 1;

    // The address the server binds to while remote connections are off, and where clients look by default
    static constexpr const CHAR* LoopbackAddress = "127.0.0.1";

    enum class EMessageType : uint32
    {
        Hello          = 1,
        Rejected       = 2,
        CompileRequest = 3,
        CompileResult  = 4,
        Ping           = 5,
        Pong           = 6,
    };

    struct FFrameHeader
    {
        uint32       Magic       = RemoteShaderCompilerProtocol::Magic;
        EMessageType Type        = EMessageType::Ping;
        uint32       RequestId   = 0;
        uint32       HeaderSize  = 0;
        uint32       PayloadSize = 0;
    };

    static_assert(sizeof(FFrameHeader) == 20, "FFrameHeader is serialized as-is, bump Version when it changes");

    struct FFrame
    {
        EMessageType  Type      = EMessageType::Ping;
        uint32        RequestId = 0;
        FJsonValue    Header;
        TArray<uint8> Payload;
    };

    static constexpr const CHAR* StatusSucceeded   = "succeeded";
    static constexpr const CHAR* StatusFailed      = "failed";      // The shader has an error, compiling it elsewhere gives the same result.
    static constexpr const CHAR* StatusUnavailable = "unavailable"; // The server could not serve the language, the requester may compile it itself.

    /** @brief Appends the encoded frame to OutBytes */
    static void EncodeFrame(const FFrame& Frame, TArray<uint8>& OutBytes);

    /**
     * @brief Moves the first complete frame out of the front of InOutBuffer. bOutHasFrame is false while the frame is still incomplete.
     * @return False for a bad magic, an unknown type, a size past the limits or a header that is not a JSON object, meaning the peer should be dropped
     */
    static bool TryDecodeFrame(TArray<uint8>& InOutBuffer, FFrame& OutFrame, bool& bOutHasFrame);

    /** @return False for anything but a relative path of plain names: no absolute paths, drive letters, backslashes, '.', '..' or empty segments */
    NODISCARD static bool IsValidSourcePath(const String& Path);

    /** @return Path with '\' turned into '/', "." segments dropped and "Name/.." pairs collapsed, so the same file always has the same name */
    NODISCARD static String CollapsePath(const String& Path);

    NODISCARD static FJsonValue IdentityToJson(const FShaderCompilerIdentity& Identity);
    static bool IdentityFromJson(const FJsonValue& Value, FShaderCompilerIdentity& OutIdentity);

    /** @return "Windows" or "macOS" */
    NODISCARD static const CHAR* GetPlatformName();
};

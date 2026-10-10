#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Json/JsonValue.h"
#include "Core/Misc/IOutputDevice.h"

struct CORE_API RemoteConsoleProtocol
{
    static constexpr uint16 DefaultPort       = 9370;
    static constexpr uint16 MinPort           = 1;
    static constexpr uint16 MaxPort           = 65535;
    static constexpr int32  Version           = 1;
    static constexpr int32  MaxLineLength     = 64 * 1024;       // A client that sends a longer line is dropped
    static constexpr int32  MaxSendBufferSize = 4 * 1024 * 1024; // A client that stops reading is dropped past this

    // Requests sent by the client
    static constexpr const CHAR* TypeExec         = "exec";
    static constexpr const CHAR* TypeList         = "list";
    static constexpr const CHAR* TypeGet          = "get";
    static constexpr const CHAR* TypeSubscribeLog = "subscribe_log";
    static constexpr const CHAR* TypePing         = "ping";

    // Messages sent by the server
    static constexpr const CHAR* TypeHello    = "hello";
    static constexpr const CHAR* TypeResult   = "result";
    static constexpr const CHAR* TypeLog      = "log";
    static constexpr const CHAR* TypeRejected = "rejected";

    // The address the server binds to while remote connections are off, and where the tool looks by default
    static constexpr const CHAR* LoopbackAddress = "127.0.0.1";

    // Console objects with this prefix configure the server itself, so remote clients may not touch them
    static constexpr const CHAR* SettingsPrefix = "RemoteConsole.";

    static const CHAR* SeverityToString(ELogSeverity Severity);
    static bool        SeverityFromString(const String& Text, ELogSeverity& OutSeverity);

    /**
     * @brief Moves every complete '\n'-terminated line from the front of Buffer into OutLines, stripping any '\r'.
     * @return False when the unterminated remainder is longer than MaxLineLength, meaning the peer should be dropped
     */
    static bool ExtractLines(String& Buffer, TArray<String>& OutLines);

    /** @return Message written as compact JSON followed by '\n' */
    static String ToLine(const FJsonValue& Message);
};

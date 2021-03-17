#pragma once

template<typename T>
inline T GetTypedProcAddress(HMODULE hModule, LPCSTR lpProcName)
{
    T Func = reinterpret_cast<T>(GetProcAddress(hModule, lpProcName));
    if (!Func)
    {
        const char* ProcName = lpProcName;
        LOG_ERROR("Failed to load " + std::string(ProcName));
    }

    return Func;
}
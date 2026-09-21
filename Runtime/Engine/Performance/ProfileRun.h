#pragma once

struct ENGINE_API FProfileRun
{
    /** @brief Reads profile-run options after the command line has been initialized. */
    static void ConfigureFromCommandLine();

    /** @brief Configures the boot and frame profilers before boot-time work is traced. */
    static void PrepareCapture();

    /** @brief Starts the timed CPU and GPU capture after engine initialization completes. */
    static void BeginCapture();

    /** @brief Completes an elapsed capture, writes its reports, and requests engine exit. */
    static void Tick();
};

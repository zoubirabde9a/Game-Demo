/* Notices Ctrl+C and service stops so the server can say goodbye to its
   players before exiting. Linux: SIGINT and SIGTERM (what systemd sends).
   Windows: Ctrl+C, Ctrl+Break and closing the console window. */

global_variable volatile bool32 GlobalStopRequested;

#if defined(_WIN32)

internal BOOL WINAPI
StopSignalHandler(DWORD Event)
{
    (void)Event;
    GlobalStopRequested = true;
    // The process is killed once this returns after a console close, so
    // give the main loop one tick to send its goodbyes.
    Sleep(100);
    return TRUE;
}

internal void
StopSignalInstall()
{
    SetConsoleCtrlHandler(StopSignalHandler, TRUE);
}

#else

#include <signal.h>

internal void
StopSignalHandler(int Signal)
{
    (void)Signal;
    GlobalStopRequested = true;
}

internal void
StopSignalInstall()
{
    signal(SIGINT, StopSignalHandler);
    signal(SIGTERM, StopSignalHandler);
}

#endif

internal bool32
StopSignalReceived()
{
    return GlobalStopRequested;
}

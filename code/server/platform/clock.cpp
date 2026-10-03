/* Wall clock and sleep for the server loop. ClockSeconds counts from an
   arbitrary start; only differences between two calls mean anything. */

#if defined(_WIN32)

#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

internal double
ClockSeconds()
{
    LARGE_INTEGER Frequency, Counter;
    QueryPerformanceFrequency(&Frequency);
    QueryPerformanceCounter(&Counter);
    return (double)Counter.QuadPart / (double)Frequency.QuadPart;
}

internal void
ClockSleep(double Seconds)
{
    // Without this Windows rounds Sleep up to ~16 ms, a whole tick.
    local_persist bool32 FineScheduler = (timeBeginPeriod(1), true);
    (void)FineScheduler;
    Sleep((DWORD)(Seconds * 1000.0));
}

#else

#include <time.h>

internal double
ClockSeconds()
{
    timespec Now;
    clock_gettime(CLOCK_MONOTONIC, &Now);
    return (double)Now.tv_sec + (double)Now.tv_nsec * 1e-9;
}

internal void
ClockSleep(double Seconds)
{
    timespec Wait;
    Wait.tv_sec = (time_t)Seconds;
    Wait.tv_nsec = (long)((Seconds - (double)Wait.tv_sec) * 1e9);
    nanosleep(&Wait, 0);
}

#endif

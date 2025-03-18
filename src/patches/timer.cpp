#include "constants.h"
#include "helpers.h"
#include "patches.h"

extern "C" NTSTATUS NTAPI NtSetTimerResolution(
  IN ULONG DesiredResolution,
  IN BOOLEAN SetResolution,
  OUT PULONG CurrentResolution
);

extern "C" NTSTATUS NTAPI NtQueryTimerResolution(
  OUT PULONG CurrentResolution,
  OUT PULONG MinimumResolution,
  OUT PULONG MaximumResolution
);

namespace patches::Timer {
    ULONG currentResolution = 0;
    ULONG minimumResolution = 0;
    ULONG maximumResolution = 0;

    void
    Release () {
        NtSetTimerResolution(currentResolution, FALSE, &currentResolution);
    }

    void
    Init () {
        NTSTATUS queryStatus = NtQueryTimerResolution(&currentResolution, &minimumResolution, &maximumResolution);
        if (queryStatus == 0) {
            bool setHighResolutionTimer = currentResolution > maximumResolution;

            if (setHighResolutionTimer) {
                ULONG targetResolution = currentResolution;
                NTSTATUS status = NtSetTimerResolution(maximumResolution, TRUE, &targetResolution);
                if (status == 0) {
                    LogMessage (LogLevel::INFO, "Using HiRes Timer: {}ms -> {}ms", currentResolution / 10000.0, targetResolution / 10000.0);
                } else {
                    LogMessage (LogLevel::WARN, "Failed to change Timer resolution, status={}", status);
                }
                atexit (Release);
            }
        } else LogMessage (LogLevel::WARN, "Failed to query Timer resolution, status={}", queryStatus);
    }
}
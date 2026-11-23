// The Clock screen's set, with no LVGL and no board in it, so tests/host checks it on the computer: which moment is written (the hour
// and minute the person chose, seconds at 0, and the date the RTC holds when Set clock is pressed — read again just before the write, so
// a set after midnight keeps today's), what the screen shows after it, and that a failed set changes nothing the person chose.

#pragma once

#include <cstddef>
#include <cstdio>
#include <ctime>

namespace hw_test::clock_rules {

// A clock that reads nothing (never set, or its oscillator stopped) starts a set from Thursday 2026-01-01.
inline std::tm DefaultDate()
{
    std::tm date{};
    date.tm_year = 2026 - 1900;
    date.tm_mday = 1;
    date.tm_wday = 4;
    return date;
}

// The moment to write: the date the RTC holds now (`now`, null when it would not read), else the date last read (`kept`, null when
// none was), else the default; the chosen `hour` and `minute`, seconds 0.
inline std::tm When(const std::tm *now, const std::tm *kept, int hour, int minute)
{
    std::tm when = now != nullptr ? *now : kept != nullptr ? *kept : DefaultDate();
    when.tm_hour = hour;
    when.tm_min = minute;
    when.tm_sec = 0;
    return when;
}

struct SetOutcome {
    bool ok = false;
    std::tm shown{};  // after a set: what "Now" shows — the RTC read back, or the moment written when it will not read back
    std::tm date{};   // the date to keep for the next set
    bool dated = false;
};

// Set clock: `read(std::tm *)` reads the RTC and `write(const std::tm &)` writes it, each returning whether it worked. The screen's
// rollers are not an input to what comes back, and nothing here changes them: a failed set leaves what the person chose as it was.
template <class Read, class Write>
SetOutcome Set(Read read, Write write, const std::tm *kept, int hour, int minute)
{
    SetOutcome out;
    std::tm now{};
    const bool have_now = read(&now);
    const std::tm when = When(have_now ? &now : nullptr, kept, hour, minute);
    out.date = when;
    out.dated = have_now || kept != nullptr;
    if (!write(when)) {
        return out;
    }
    out.ok = true;
    out.shown = when;
    std::tm back{};
    if (read(&back)) {
        out.shown = back;
        out.date = back;
    }
    out.dated = true;
    return out;
}

inline void SetWords(char *out, size_t size, const std::tm &when)
{
    std::snprintf(out, size, "Clock set to %02d:%02d", when.tm_hour, when.tm_min);
}

inline void NowWords(char *out, size_t size, const std::tm &when)
{
    std::snprintf(out, size, "Now %02d:%02d", when.tm_hour, when.tm_min);
}

}  // namespace hw_test::clock_rules

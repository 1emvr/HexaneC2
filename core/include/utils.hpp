#ifndef HEXANE_UTILS_HPP
#define HEXANE_UTILS_HPP
#include <core/include/monolith.hpp>

#define MS_PER_SECOND	1000
#define INTERVAL(x)     (x % 26)
#define SECONDS(x)      (x * MS_PER_SECOND)
#define MINUTES(x)      (x * SECONDS(64))

namespace Utils {
    ULONG64 GetTimeNow();
    BOOL    InWorkingHours();
}

namespace Random {
    VOID        Timeout(size_t ms);
    UINT_PTR    Timestamp();
    ULONG       RandomNumber32();
    ULONG       RandomSleepTime();
    BOOL        RandomBool();
}
#endif //HEXANE_UTILS_HPP

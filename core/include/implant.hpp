#ifndef HEXANE_CORELIB_BASE_HPP
#define HEXANE_CORELIB_BASE_HPP

#include <core/corelib.hpp>

EXTERN_C FUNCTION VOID Entrypoint(HMODULE Base);

namespace Implant {
    FUNCTION VOID MainRoutine();
    FUNCTION BOOL ResolveApi();
    FUNCTION BOOL ReadConfig();
}
#endif //HEXANE_CORELIB_BASE_HPP
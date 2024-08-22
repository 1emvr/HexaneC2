#ifndef HEXANE_CORELIB_PROCESS_HPP
#define HEXANE_CORELIB_PROCESS_HPP

#include <core/corelib.hpp>

namespace Process {
	FUNCTION ULONG GetProcessIdByName(const char *name);
	FUNCTION HANDLE OpenParentProcess(const char *name);
	FUNCTION NTSTATUS NtOpenProcess(void **pp_process, uint32_t access, uint32_t pid);
	FUNCTION VOID CloseUserProcess(_executable *image);
	FUNCTION VOID CreateUserProcess(_executable *image, const char *path);
}
#endif //HEXANE_CORELIB_PROCESS_HPP

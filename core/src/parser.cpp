#include <core/include/parser.hpp>
namespace Parser {

    VOID ParserBytecpy(PPARSER Parser, PBYTE dst) {
        HEXANE

        BYTE byte = Parser::UnpackByte(Parser);
        x_memcpy(dst, &byte, 1);
    }

    VOID ParserStrcpy(PPARSER Parser, LPSTR *Dst, ULONG *cbOut) {
        HEXANE

        ULONG Length  = 0;
        LPSTR Buffer  = UnpackString(Parser, &Length);

        if (Length) {
            if (cbOut) {
                *cbOut = Length;
            }

            if ((*Dst = S_CAST(LPSTR, Ctx->Nt.RtlAllocateHeap(Ctx->Heap, 0, Length)))) {
                x_memcpy(*Dst, Buffer, Length);
            }
        }
    }

    VOID ParserWcscpy(PPARSER Parser, LPWSTR *Dst, ULONG *cbOut) {
        HEXANE

        ULONG Length  = 0;
        LPWSTR Buffer  = UnpackWString(Parser, &Length);

        if (Length) {
            if (cbOut) {
                *cbOut = Length;
            }

            if ((*Dst = S_CAST(LPWSTR, Ctx->Nt.RtlAllocateHeap(Ctx->Heap, 0, (Length * sizeof(WCHAR)) + sizeof(WCHAR))))) {
                x_memcpy(*Dst, Buffer, Length * sizeof(WCHAR));
            }
        }
    }

    VOID ParserMemcpy(PPARSER Parser, PBYTE *Dst, ULONG *cbOut) {
        HEXANE

        ULONG Length = 0;
        PBYTE Buffer = UnpackBytes(Parser, &Length);

        if (Length) {
            if (cbOut) {
                *cbOut = Length;
            }

            if ((*Dst = S_CAST(PBYTE, Ctx->Nt.RtlAllocateHeap(Ctx->Heap, 0, Length)))) {
                x_memcpy(*Dst, Buffer, Length);
            }
        }
    }

    VOID CreateParser (PPARSER Parser, PBYTE Buffer, ULONG Length) {
        HEXANE

        if (!(Parser->Handle = Ctx->Nt.RtlAllocateHeap(Ctx->Heap, 0, Length))) {
            return;
        }

        x_memcpy(Parser->Handle, Buffer, Length);

        Parser->Length = Length;
        Parser->Buffer = Parser->Handle;
        Parser->LE = Ctx->LE;
    }

    VOID DestroyParser (PPARSER Parser) {
        HEXANE

        if (Parser) {
            if (Parser->Handle) {

                x_memset(Parser->Handle, 0, Parser->Length);
                Ctx->Nt.RtlFreeHeap(Ctx->Heap, 0, Parser->Handle);

                Parser->Buffer = nullptr;
                Parser->Handle = nullptr;
            }
        }
    }

    BYTE UnpackByte (PPARSER Parser) {
        BYTE intBytes = 0;

        if (Parser->Length >= 1) {
            x_memcpy(&intBytes, Parser->Buffer, 1);

            Parser->Buffer = S_CAST(PBYTE, Parser->Buffer) + 1;
            Parser->Length -= 1;
        }

        return intBytes;
    }

    SHORT UnpackShort (PPARSER Parser) {

        SHORT intBytes = 0;

        if (Parser->Length >= 2) {
            x_memcpy(&intBytes, Parser->Buffer, 2);

            Parser->Buffer = S_CAST(PBYTE, Parser->Buffer) + 2;
            Parser->Length -= 2;
        }
        return intBytes;
    }

    ULONG UnpackDword (PPARSER Parser) {

        ULONG intBytes = 0;

        if (!Parser || Parser->Length < 4) {
            return 0;
        }
        x_memcpy(&intBytes, Parser->Buffer, 4);

        Parser->Buffer = S_CAST(PBYTE, Parser->Buffer) + 4;
        Parser->Length -= 4;

        return (Parser->LE)
               ? intBytes
               : __bswapd(S_CAST(ULONG, intBytes));
    }

    ULONG64 UnpackDword64 (PPARSER Parser) {

        ULONG64 intBytes = 0;

        if (!Parser || Parser->Length < 8) {
            return 0;
        }
        x_memcpy(&intBytes, Parser->Buffer, 4);

        Parser->Buffer = S_CAST(PBYTE, Parser->Buffer) + 8;
        Parser->Length -= 8;

        return (Parser->LE)
               ? intBytes
               : __bswapq(S_CAST(ULONG64, intBytes));
    }

    BOOL UnpackBool (PPARSER Parser) {

        INT32 intBytes = 0;

        if (!Parser || Parser->Length < 4) {
            return 0;
        }
        x_memcpy(&intBytes, Parser->Buffer, 4);

        Parser->Buffer = S_CAST(PBYTE, Parser->Buffer) + 4;
        Parser->Length -= 4;

        return (Parser->LE)
               ? intBytes != 0
               : __bswapd(intBytes) != 0;
    }

    PBYTE UnpackBytes (PPARSER Parser, PULONG cbOut) {

        ULONG length = 0;
        PBYTE output = { };

        if (!Parser || Parser->Length < 4) {
            return nullptr;
        }

        length = UnpackDword(Parser);
        if (cbOut) {
            *cbOut = length;
        }

        output = S_CAST(PBYTE, Parser->Buffer);
        if (output == nullptr) {
            return nullptr;
        }

        Parser->Length -= length;
        Parser->Buffer = S_CAST(PBYTE, Parser->Buffer) + length;

        return output;
    }

    LPSTR UnpackString(PPARSER Parser, PULONG cbOut) {
        return R_CAST(LPSTR, UnpackBytes(Parser, cbOut));
    }

    LPWSTR UnpackWString(PPARSER Parser, PULONG cbOut) {
        return R_CAST(LPWSTR, UnpackBytes(Parser, cbOut));
    }
}

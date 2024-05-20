#include "c_ffi.h"

extern "C" {

static std::vector<PCSX2Ipc::BatchCommand *> batch_commands;

PCSX2Ipc *pcsx2ipc_new() { return new PCSX2Ipc(); }

PCSX2Ipc::BatchCommand pcsx2ipc_internal_to_batch(PCSX2Ipc::BatchCommand *arg) {
    return PCSX2Ipc::BatchCommand{
        PCSX2Ipc::IPCBuffer{ arg->ipc_message.size, arg->ipc_message.buffer },
        PCSX2Ipc::IPCBuffer{ arg->ipc_return.size, arg->ipc_return.buffer },
        arg->return_locations
    };
}

void pcsx2ipc_initialize_batch(PCSX2Ipc *v) { return v->InitializeBatch(); }

int pcsx2ipc_finalize_batch(PCSX2Ipc *v) {
    auto p_batch = new PCSX2Ipc::BatchCommand;
    auto batch = v->FinalizeBatch();
    p_batch->ipc_message = batch.ipc_message;
    p_batch->ipc_return = batch.ipc_return;
    p_batch->return_locations = batch.return_locations;
    batch_commands.push_back(p_batch);
    return batch_commands.size() - 1;
}

uint64_t pcsx2ipc_get_reply_int(PCSX2Ipc *v, int cmd, int place,
                                PCSX2Ipc::IPCCommand msg) {
    auto lcmd = pcsx2ipc_internal_to_batch(batch_commands[cmd]);
    switch (msg) {
        case PCSX2Ipc::MsgRead8:
            return (uint64_t)v->GetReply<PCSX2Ipc::MsgRead8>(lcmd, place);
        case PCSX2Ipc::MsgRead16:
            return (uint64_t)v->GetReply<PCSX2Ipc::MsgRead16>(lcmd, place);
        case PCSX2Ipc::MsgRead32:
            return (uint64_t)v->GetReply<PCSX2Ipc::MsgRead32>(lcmd, place);
        case PCSX2Ipc::MsgRead64:
            return v->GetReply<PCSX2Ipc::MsgRead64>(lcmd, place);
        default:
            return 0;
    }
}

void pcsx2ipc_send_command(PCSX2Ipc *v, int cmd) {
    return v->SendCommand(pcsx2ipc_internal_to_batch(batch_commands[cmd]));
}

uint64_t pcsx2ipc_read(PCSX2Ipc *v, uint32_t address, PCSX2Ipc::IPCCommand msg,
                       bool batch) {
    if (batch == false) {
        switch (msg) {
            case PCSX2Ipc::MsgRead8:
                return (uint64_t)v->Read<uint8_t>(address);
            case PCSX2Ipc::MsgRead16:
                return (uint64_t)v->Read<uint16_t>(address);
            case PCSX2Ipc::MsgRead32:
                return (uint64_t)v->Read<uint32_t>(address);
            case PCSX2Ipc::MsgRead64:
                return v->Read<uint64_t>(address);
            default:
                return 0;
        }
    } else {
        switch (msg) {
            case PCSX2Ipc::MsgRead8:
                v->Read<uint8_t, true>(address);
                return 0;
            case PCSX2Ipc::MsgRead16:
                v->Read<uint16_t, true>(address);
                return 0;
            case PCSX2Ipc::MsgRead32:
                v->Read<uint32_t, true>(address);
                return 0;
            case PCSX2Ipc::MsgRead64:
                v->Read<uint64_t, true>(address);
                return 0;
            default:
                return 0;
        }
    }
}

void pcsx2ipc_write(PCSX2Ipc *v, uint32_t address, uint8_t val,
                    PCSX2Ipc::IPCCommand msg, bool batch) {
    if (batch == false) {
        switch (msg) {
            case PCSX2Ipc::MsgWrite8:
                v->Write<uint8_t>(address, val);
                break;
            case PCSX2Ipc::MsgWrite16:
                v->Write<uint16_t>(address, val);
                break;
            case PCSX2Ipc::MsgWrite32:
                v->Write<uint32_t>(address, val);
                break;
            case PCSX2Ipc::MsgWrite64:
                v->Write<uint64_t>(address, val);
                break;
            default:
                break;
        }
    } else {
        switch (msg) {
            case PCSX2Ipc::MsgWrite8:
                v->Write<uint8_t, true>(address, val);
                break;
            case PCSX2Ipc::MsgWrite16:
                v->Write<uint16_t, true>(address, val);
                break;
            case PCSX2Ipc::MsgWrite32:
                v->Write<uint32_t, true>(address, val);
                break;
            case PCSX2Ipc::MsgWrite64:
                v->Write<uint64_t, true>(address, val);
                break;
            default:
                break;
        }
    }
}

char *pcsx2ipc_version(PCSX2Ipc *v, bool batch) { return v->Version(); }

PCSX2Ipc::IPCStatus pcsx2ipc_get_error(PCSX2Ipc *v) { return v->GetError(); }

void pcsx2ipc_free_batch_command(int cmd) {
    if (batch_commands[cmd] != NULL) {
        delete[] batch_commands[cmd]->ipc_message.buffer;
        delete[] batch_commands[cmd]->ipc_return.buffer;
        delete[] batch_commands[cmd]->return_locations;
        delete batch_commands[cmd];
        batch_commands[cmd] = NULL;
    }
}
void pcsx2ipc_delete(PCSX2Ipc *v) {
    for (int i = 0; i < batch_commands.size(); i++)
        pcsx2ipc_free_batch_command(i);
    delete v;
}
}

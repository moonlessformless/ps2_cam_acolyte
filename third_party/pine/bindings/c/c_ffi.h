#pragma once

/** \file c_ffi.h
 * This is the C bindings definition of the PCSX2 IPC API. @n
 * The C bindings of the API are inherently unsafe. You _can_ shoot yourself in
 * the foot, and probably will. @n
 * I encourage you to carefully read the documentation for the C++
 * library on top of the C bindings to have a proper understanding of what each
 * function does, and how the binding differs from the source material.
 */

#include "pcsx2_ipc.h"
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @see PCSX2Ipc::PCSX2Ipc
 */
PCSX2Ipc *pcsx2ipc_new();

/**
 * @see PCSX2Ipc::InitializeBatch
 */
void pcsx2ipc_initialize_batch(PCSX2Ipc *v);

/**
 * In contrast to the C++ library this returns a handle to a struct. @n
 * This requires you to free handles by yourself, see
 * pcsx2ipc_free_batch_command.
 * @see pcsx2ipc_free_batch_command
 * @return PCSX2Ipc::BatchCommand handle.
 * @see PCSX2Ipc::FinalizeBatch
 */
int pcsx2ipc_finalize_batch(PCSX2Ipc *v);

/**
 * Internal undocumented function. Do not use or include into your bindings!
 */
PCSX2Ipc::BatchCommand pcsx2ipc_internal_to_batch(PCSX2Ipc::BatchCommand *arg);

/**
 * Variant of PCSX2Ipc::GetReply that exclusively deals with integers replies.
 * @see PCSX2Ipc::GetReply
 */
uint64_t pcsx2ipc_get_reply_int(PCSX2Ipc *v, int cmd, int place,
                                PCSX2Ipc::IPCCommand msg);

/**
 * @see PCSX2Ipc::SendCommand
 */
void pcsx2ipc_send_command(PCSX2Ipc *v, int cmd);

/**
 * @see PCSX2Ipc::Read
 */
uint64_t pcsx2ipc_read(PCSX2Ipc *v, uint32_t address, PCSX2Ipc::IPCCommand msg,
                       bool batch);

/**
 * @see PCSX2Ipc::Version
 */
char *pcsx2ipc_version(PCSX2Ipc *v, bool batch);

/**
 * @see PCSX2Ipc::Write
 */
void pcsx2ipc_write(PCSX2Ipc *v, uint32_t address, uint8_t val,
                    PCSX2Ipc::IPCCommand msg, bool batch);

/**
 * @see PCSX2Ipc::~PCSX2Ipc
 */
void pcsx2ipc_delete(PCSX2Ipc *v);

/**
 * Frees given PCSX2Ipc::BatchCommand through its int handle. @n
 * As the C bindings handle structures for you, you have to tell them when to
 * free the batch commands if you want to free memory.
 * @param cmd PCSX2Ipc::BatchCommand handle.
 */
void pcsx2ipc_free_batch_command(int cmd);
/**
 * @see PCSX2Ipc::GetError
 */
PCSX2Ipc::IPCStatus pcsx2ipc_get_error(PCSX2Ipc *v);

#ifdef __cplusplus
}
#endif

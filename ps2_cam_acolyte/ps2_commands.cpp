#include "ps2_commands.h"
#include "ps2.h"
#include <iostream>

using enum PINE::PCSX2::IPCCommand;

ps2_ipc_cmd::ps2_ipc_cmd(const pcsx2& ps2)
	: ipc(ps2.get_ipc())
{
	std::memset(&batch_cmd, 0, sizeof(PINE::Shared::BatchCommand));
}

void ps2_ipc_cmd::init_if_necessary()
{
	if (pending_initialize)
	{
		pending_initialize = false;
		ipc->InitializeBatch();
	}
}

ps2_ipc_cmd& ps2_ipc_cmd::finalize()
{
	assert(!pending_initialize);
	assert(pending_finalize);
	pending_finalize = false;
	batch_cmd = ipc->FinalizeBatch();
	return *this;
}

ps2_ipc_cmd& ps2_ipc_cmd::send()
{
	assert(!pending_initialize);
	if (pending_finalize)
	{
		finalize();
	}

	pending_send = false;

	try
	{
		ipc->SendCommand(batch_cmd);
	}
	catch (const PINE::PCSX2::IPCStatus& status)
	{
		std::cerr << "IPC error: " << status;
	}

	return *this;
}

ps2_ipc_cmd::~ps2_ipc_cmd()
{
	assert(pending_initialize || !pending_finalize);
	batch_cmd.Free();
}

sentinel_counter::sentinel_counter(const pcsx2& ps2, size_t address)
	: ps2(ps2)
	, address(address)
	, read(ps2)
{
	last_value = 1; // force a reset on first update
	read.queue_read<uint32_t>(address);
	read.finalize();
}

bool sentinel_counter::has_reset()
{
	read.send();
	uint32_t new_value = read.read(read_value);
	if (new_value != last_value)
	{
		last_value = new_value;
		return true;
	}
	else
	{
		return false;
	}
}

void sentinel_counter::increment()
{
	++last_value;
	ps2_ipc_cmd write(ps2);
	write.write<uint32_t>(address, last_value).finalize().send();
}

toggle_state::toggle_state(const pcsx2& ps2)
	: ps2(ps2)
	, on_cmd(ps2)
	, off_cmd(ps2)
{
}

void toggle_state::toggle()
{
	on = !on;
	if (on) { on_cmd.send(); }
	if (!on) { off_cmd.send(); }
}

void toggle_state::set_on(bool on)
{
	if (this->on != on)
	{
		toggle();
	}
}

void toggle_state::reset()
{
	on = false;
	off_cmd.send();
}

multi_toggle_state::multi_toggle_state(const pcsx2& ps2, size_t state_count)
	: ps2(ps2)
{
	cmds.reserve(state_count);
	for (int i = 0; i < state_count; ++i)
	{
		cmds.emplace_back(ps2);
	}
}

void multi_toggle_state::toggle()
{
	++toggle_index;
	if (toggle_index >= cmds.size())
	{
		toggle_index = 0;
	}
	cmds[toggle_index].send();
}

void multi_toggle_state::reset()
{
	toggle_index = 0;
	cmds[toggle_index].send();
}
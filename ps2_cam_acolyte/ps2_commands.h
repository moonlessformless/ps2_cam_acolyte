#pragma once

#include <vector>
#include <cassert>
#include "pine.h"

class pcsx2;

// this is a light wrapper over PINE::PCSX2's ipc commands to make them simpler/safer
// 
// 1) create a ps2_ipc_cmd
// 2) add write/read commands
// 3) finalize it(builds the buffer)
// 4) send it(multiple times if desired)
// 
// write data:
//	cmd.write<uint8_t>(0x011B6B1B, 0x02).finalize();
// after finalizing, you can repeatedly .send() it
// 
// read data:
//	auto read_result = cmd.queue_read<uint8_t>(0x011B6B1B).finalize();
//  cmd.send();
//  uint8_t value = cmd.read(read_result);
// 
// due to PINE's memory model:
// - this class can only be moved
// - it is not threadsafe
// - you must call finalize() or send() before creating another ps2_ipc_cmd
class ps2_ipc_cmd
{
private:
	PINE::PCSX2* ipc = nullptr;
	PINE::Shared::BatchCommand batch_cmd;
	bool pending_initialize = true;
	bool pending_finalize = true;
	bool pending_send = true;
	uint8_t index = 0;

private:
	void init_if_necessary();

public:
	explicit ps2_ipc_cmd(const pcsx2& ps2);
	ps2_ipc_cmd& operator=(const ps2_ipc_cmd&) = delete;
	ps2_ipc_cmd(const ps2_ipc_cmd&) = delete;
	ps2_ipc_cmd(ps2_ipc_cmd&&) noexcept = default;
	ps2_ipc_cmd& operator=(ps2_ipc_cmd&&) noexcept = default;
	ps2_ipc_cmd& finalize();
	ps2_ipc_cmd& send();
	~ps2_ipc_cmd();
	template <typename T>
	ps2_ipc_cmd& write(uint32_t address, T value)
	{
		init_if_necessary();
		ipc->Write<T, true>(address, value);
		++index;
		return *this;
	}

	template <typename T>
	struct queued_read
	{
		uint8_t index = 0;
	};

	template <typename T>
	queued_read<T> queue_read(uint32_t address)
	{
		init_if_necessary();
		ipc->Read<T, true>(address);
		queued_read<T> result(index);
		++index;
		return result;
	}

	template <typename T>
	T read(const queued_read<T>& rc)
	{
		assert(!pending_send);
		constexpr PINE::Shared::IPCCommand tag = []() -> PINE::Shared::IPCCommand {
			switch (sizeof(T)) {
			case 1:
				return PINE::Shared::MsgRead8;
			case 2:
				return PINE::Shared::MsgRead16;
			case 4:
				return PINE::Shared::MsgRead32;
			case 8:
				return PINE::Shared::MsgRead64;
			default:
				return PINE::Shared::MsgUnimplemented;
			}
		}();
		static_assert(tag != PINE::Shared::MsgUnimplemented);
		auto reply = ipc->GetReply<tag>(batch_cmd, rc.index);
		T result;
		memcpy(&result, &reply, sizeof(T));
		return result;
	}
};

class sentinel_counter
{
private:
	const pcsx2& ps2;
	size_t address = 0;
	uint32_t last_value = 0;
	ps2_ipc_cmd read;
	ps2_ipc_cmd::queued_read<size_t> read_value;

public:
	sentinel_counter(const pcsx2& ps2, size_t address);
	bool has_reset();
	void increment();
};

class toggle_state
{
private:
	const pcsx2& ps2;
	ps2_ipc_cmd on_cmd;
	ps2_ipc_cmd off_cmd;
	bool on = false;

public:
	explicit toggle_state(const pcsx2& ps2);
	ps2_ipc_cmd& edit_on() { return on_cmd; }
	ps2_ipc_cmd& edit_off() { return off_cmd; }
	void toggle();
	void set_on(bool on);
	bool is_on() const { return on; }
	void reset();
};

class multi_toggle_state
{
private:
	const pcsx2& ps2;
	std::vector<ps2_ipc_cmd> cmds;
	size_t toggle_index = 0;

public:
	multi_toggle_state(const pcsx2& ps2, size_t state_count);
	ps2_ipc_cmd& edit_cmd(size_t index) { return cmds[index]; }
	void toggle();
	size_t current_index() const { return toggle_index; }
	void reset();
};

template <typename T, size_t N>
class read_only_value_set
{
private:
	const pcsx2& ps2;
	ps2_ipc_cmd read_all;

	struct read_value
	{
		T last_value = {};
		uint32_t address = 0;
		ps2_ipc_cmd::queued_read<T> read_cmd;
	};

	std::array<read_value, N> v;

public:
	read_only_value_set(const pcsx2& ps2, const std::array<uint32_t, N>& addresses)
		: ps2(ps2)
		, read_all(ps2)
	{
		for (int i = 0; i < N; ++i)
		{
			v[i].address = addresses[i];
			v[i].read_cmd = read_all.queue_read<T>(addresses[i]);
		}
		read_all.finalize();
	}

	void update()
	{
		read_all.send();
		for (int i = 0; i < N; ++i)
		{
			v[i].last_value = read_all.read<T>(v[i].read_cmd);
		}
	}

	T get(size_t index)
	{
		return v[index].last_value;
	}
};

template <typename T, size_t N>
class tweakable_value_set
{
private:
	const pcsx2& ps2;
	ps2_ipc_cmd read_all;

	struct tweakable_value
	{
		T last_set_value = {};
		T last_untweaked_value = {};
		uint32_t address = 0;
		ps2_ipc_cmd::queued_read<T> read_cmd;
	};

	std::array<tweakable_value, N> v;

	bool is_tweaking = false;

public:
	tweakable_value_set(const pcsx2& ps2, const std::array<uint32_t, N>& addresses)
		: ps2(ps2)
		, read_all(ps2)
	{
		for (int i = 0; i < N; ++i)
		{
			v[i].address = addresses[i];
			v[i].read_cmd = read_all.queue_read<T>(addresses[i]);
		}
		read_all.finalize();
	}

	tweakable_value_set(const pcsx2& ps2, uint32_t base_address)
		: ps2(ps2)
		, read_all(ps2)
	{
		for (int i = 0; i < N; ++i)
		{
			v[i].address = base_address + i * sizeof(T);
			v[i].read_cmd = read_all.queue_read<T>(v[i].address);
		}
		read_all.finalize();
	}

	bool currently_tweaking() const
	{
		return is_tweaking;
	}

	void start_tweaking()
	{
		assert(!is_tweaking);
		read_all.send();
		for (int i = 0; i < N; ++i)
		{
			v[i].last_untweaked_value = read_all.read<T>(v[i].read_cmd);
			v[i].last_set_value = v[i].last_untweaked_value;
		}
		is_tweaking = true;
	}

	void stop_tweaking(bool restore_values = true)
	{
		assert(is_tweaking);
		if (restore_values)
		{
			for (int i = 0; i < N; ++i)
			{
				v[i].last_set_value = v[i].last_untweaked_value;
			}
			flush(ps2);
		}
		is_tweaking = false;
	}

	void toggle_tweaking()
	{
		if (is_tweaking) stop_tweaking(ps2);
		else start_tweaking();
	}

	T get(size_t index)
	{
		return v[index].last_set_value;
	}

	void set(size_t index, T value)
	{
		assert(is_tweaking);
		v[index].last_set_value = value;
	}

	void add(size_t index, T delta)
	{
		assert(is_tweaking);
		v[index].last_set_value += delta;
	}

	void flush(const pcsx2& ps2)
	{
		assert(is_tweaking);
		// with some modificiations to PINE, we could re-use the buffer
		// but for now we assemble a new command
		ps2_ipc_cmd cmd(ps2);
		for (int i = 0; i < N; ++i)
		{
			cmd.write<T>(v[i].address, v[i].last_set_value);
		}
		cmd.send();
	}

	void reset()
	{
		is_tweaking = false;
		start_tweaking();
		stop_tweaking(false);
	}
};

template <size_t N>
class restoring_toggle_state
{
public:
	struct restorable_value
	{
		uint32_t address;
		uint32_t tweaked;
		uint32_t restored;
	};

private:
	const pcsx2& ps2;
	bool on = false;
	std::array<restorable_value, N> values;

public:
	// tuple is: address, tweaked value, original value
	restoring_toggle_state(const pcsx2& ps2, const std::array<restorable_value, N>& values)
		: ps2(ps2)
		, values(values)
	{
		// this will read the restored values from memory
		// use for debugging only when in a known clean state
		/*ps2_ipc_cmd read_all(ps2);
		std::array<ps2_ipc_cmd::queued_read<uint32_t>, N> reads;
		for (int i = 0; i < N; ++i)
		{
			reads[i] = read_all.queue_read<uint32_t>(values[i].address);
		}
		read_all.send();
		for (int i = 0; i < N; ++i)
		{
			uint32_t passed_restore_value = values[i].restored;
			uint32_t actual_value = read_all.read(reads[i]);
			if (passed_restore_value != actual_value)
			{
				values[i].restored = actual_value;
			}
		}*/
	}
	void set_on(bool on)
	{
		this->on = on;
		ps2_ipc_cmd cmd(ps2);
		for (int i = 0; i < N; ++i)
		{
			cmd.write<uint32_t>(values[i].address, on ? values[i].tweaked : values[i].restored);
		}
		cmd.send();
	}
	void toggle()
	{
		set_on(!on);
	}
	bool is_on() const { return on; }
	void reset()
	{
		set_on(false);
	}
};
#pragma once

#include "controller.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <functional>
#include <algorithm>

class ps2_game;
class pcsx2;

class ps2_game_registry
{
public:
	using factory = std::function<std::unique_ptr<ps2_game>(const pcsx2&)>;

private:
	struct game_entry
	{
		const char* name = nullptr;
		factory f;
	};
	using map = std::unordered_map<std::string, game_entry>;
	static map& name_to_factory()
	{
		static map ntf;
		return ntf;
	}

public:
	template <typename T>
	static void register_ps2_game(const char* uuid, const char* name, const factory& f)
	{
		name_to_factory()[uuid] = { .name = name, .f = f };
	}

	static const char* name_for_uuid(const char* uuid)
	{
		if (name_to_factory().contains(uuid))
		{
			return name_to_factory()[uuid].name;
		}
		else
		{
			return nullptr;
		}
	}

	static std::unique_ptr<ps2_game> create_ps2_game(const char* uuid, const pcsx2& p)
	{
		if (name_to_factory().contains(uuid))
		{
			return name_to_factory()[uuid].f(p);
		}
		else
		{
			return nullptr;
		}
	}

	using ps2_game_info = std::pair<std::string, std::string>; // uuid, game name
	static const std::vector<ps2_game_info>& list_ps2_games()
	{
		static std::vector<ps2_game_info> list;
		if (list.empty())
		{
			list.reserve(name_to_factory().size());
			std::transform(name_to_factory().begin(), name_to_factory().end(), std::back_inserter(list),
				[](const auto& pair) {
					return std::pair(pair.first, pair.second.name);
				});
			std::erase_if(list, [](const ps2_game_info& i) { return i.first == "00000000"; });
			std::sort(list.begin(), list.end(), [](const ps2_game_info& a, const ps2_game_info& b) {
				return a.second < b.second;
				});
		}
		return list;
	}
};

template <typename T>
class ps2_game_static_register
{
public:
	ps2_game_static_register(const char* uuid, const char* name)
	{
		ps2_game_registry::factory f = [](const pcsx2& p) -> std::unique_ptr<ps2_game> { return std::make_unique<T>(p); };
		ps2_game_registry::register_ps2_game<T>(uuid, name, f);
	}
};

class ps2_game
{
public:
	virtual ~ps2_game() {}
	virtual void update(const pcsx2& ps2, const controller_state& state, playback& camera_playback, float time_delta) = 0;
	virtual void draw_game_ui(const pcsx2& ps2, const controller& controller, playback& camera_playback) {}
	ps2_game() = default;
	ps2_game(const ps2_game&) = delete;
	ps2_game& operator=(const ps2_game&) = delete;
};
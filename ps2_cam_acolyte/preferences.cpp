#include "preferences.h"
#include <fstream>
#include <filesystem>
#include <Windows.h>

std::wstring preferences_path()
{
	std::wstring path;
	path.resize(MAX_PATH, 0);
	auto path_size(GetModuleFileName(nullptr, &path.front(), MAX_PATH));
	path.resize(path_size);
	std::filesystem::path p(path);
	return p.remove_filename().replace_filename(L"preferences.ini");
}

std::unordered_map<std::string, std::string> read_file(const std::wstring& path)
{
	std::unordered_map<std::string, std::string> map;
	std::ifstream file(path);

	if (!file)
	{
		return map;
	}

	std::string line;

	while (std::getline(file, line))
	{
		for (int i = 0; i < line.size(); ++i)
		{
			if (line[i] == '=')
			{
				std::string key = line.substr(0, i);
				std::string value = line.substr(i + 1);
				map[key] = value;
				break;
			}
		}
	}

	file.close();

	return map;
}

void write_file(const std::wstring& path, const std::unordered_map<std::string, std::string>& values)
{
	std::ofstream file(path, std::ofstream::out);

	for (const auto& pair : values)
	{
		file << pair.first << "=" << pair.second << std::endl;
	}

	file.close();
}

preferences::preferences()
	: values(read_file(preferences_path()))
{
}

const char* preferences::read(const char* key) const
{
	prefs_map::const_iterator iter = values.find(key);

	if (iter != values.end())
	{
		return iter->second.c_str();
	}
	else
	{
		return nullptr;
	}
}

void preferences::write(const char* key, const char* value)
{
	const char* existing = read(key);
	if (existing == nullptr || strcmp(existing, value) != 0)
	{
		values[key] = value;
		write_file(preferences_path(), values);
	}
}

float preferences::read_float(const char* key, float default_value) const
{
	const char* value = read(key);
	if (value != nullptr)
	{
		float result;
		auto [ptr, ec] = std::from_chars(value, value + std::strlen(value), result);
		if (ec == std::errc())
		{
			return result;
		}
	}
	return default_value;
}

void preferences::write_float(const char* key, float value)
{
	static std::vector<char> buffer(32);
	auto [ptr, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
	if (ec == std::errc())
	{
		*ptr = '\0';
		write(key, buffer.data());
	}
}
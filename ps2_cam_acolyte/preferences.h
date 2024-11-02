#pragma once

#include <string>
#include <unordered_map>

class preferences
{
	using prefs_map = std::unordered_map<std::string, std::string>;
	prefs_map values;


public:
	preferences();
	const char* read(const char* key) const;
	void write(const char* key, const char* value);
	float read_float(const char* key, float default_value) const;
	void write_float(const char* key, float value);
	void write_on_change_float(const char* key, float* value, float target_value);
	bool read_bool(const char* key, bool default_value) const;
	void write_bool(const char* key, bool value);
	void write_on_change_bool(const char* key, bool* value, bool target_value);
};
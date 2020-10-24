//Credits: https://github.com/sol-prog
//License: https://github.com/sol-prog/cpp17-filewatcher/blob/master/LICENSE
#pragma once

#include <filesystem>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <string>
#include <functional>

// Define available file changes
enum class FileStatus { created, modified, erased };

class FileWatcher
{
public:
	std::string path_to_watch;
	// Time interval at which we check the base folder for changes
	std::chrono::duration<int, std::milli> delay;

	// Keep a record of files from the base directory and their last modification time
	FileWatcher(std::string path_to_watch, std::chrono::duration<int, std::milli> delay)
		: path_to_watch{ path_to_watch }, delay{ delay }
	{
		for (auto& file : std::filesystem::recursive_directory_iterator(path_to_watch))
		{
			_paths[file.path().string()] = std::filesystem::last_write_time(file);
		}
	}

	// Monitor "path_to_watch" for changes and in case of a change execute the user supplied "action" function
	void start(const std::function<void(std::string, FileStatus)>& action)
	{
		while (running_)
		{
			// Wait for "delay" milliseconds
			std::this_thread::sleep_for(delay);

			auto it = _paths.begin();
			while (it != _paths.end())
			{
				if (!std::filesystem::exists(it->first))
				{
					action(it->first, FileStatus::erased);
					it = _paths.erase(it);
				}
				else
				{
					++it;
				}
			}

			// Check if a file was created or modified
			for (auto& file : std::filesystem::recursive_directory_iterator(path_to_watch))
			{
				auto current_file_last_write_time = std::filesystem::last_write_time(file);

				if (endsWith(file.path().string(), ".h") || endsWith(file.path().string(), ".cpp"))
				{
					// File creation
					if (!contains(file.path().string()))
					{
						_paths[file.path().string()] = current_file_last_write_time;
						action(file.path().string(), FileStatus::created);
						// File modification
					}
					else
					{
						if (_paths[file.path().string()] != current_file_last_write_time)
						{
							_paths[file.path().string()] = current_file_last_write_time;
							action(file.path().string(), FileStatus::modified);
						}
					}
				}
			}
		}
	}

	void stop()
	{
		running_ = false;
	}
private:
	// Check if "_paths" contains a given key
	// If your compiler supports C++20 use _paths.contains(key) instead of this function
	bool contains(const std::string& key)
	{
		auto el = _paths.find(key);
		return el != _paths.end();
	}

	static bool endsWith(const std::string& str, const std::string& suffix)
	{
		return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
	}

	std::unordered_map<std::string, std::filesystem::file_time_type> _paths;
	bool running_ = true;
};
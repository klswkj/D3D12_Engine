#include "stdafx.h"
#include "OBJFileManager.h"

namespace OBJFileManager
{
	bool s_bInitialized{ false };

	std::vector<std::string> g_FileNames;
	const char* g_PathName{ nullptr };
	char g_AssetFormat[4];

	void Initialize(const char* PathName)
	{
		if (s_bInitialized)
		{
			return;
		}

		SetFormat();

		g_PathName = PathName;

		FindAllFormatFile();

		s_bInitialized = true;
	}

	void ShutDown()
	{
		g_FileNames.clear();
		g_PathName = nullptr;
	}

	void SetFormat()
	{
		static const char* typestring = "obj";

		for (size_t i{ 0 }; i < 3; ++i)
		{
			g_AssetFormat[i] = typestring[i];
		}
		g_AssetFormat[3] = '\0';
	}

	void FindAllFormatFile()
	{
		if (g_PathName == nullptr)
		{
			std::filesystem::recursive_directory_iterator itr(std::filesystem::current_path().parent_path());

			while (itr != std::filesystem::end(itr))
			{
				const std::filesystem::directory_entry& entry = *itr;
				
				std::string* PathName = &entry.path().string();
				
				if (PathName->substr(PathName->size() - 3, PathName->size()) == g_AssetFormat)
				{
					g_FileNames.push_back(*PathName);
				}
				++itr;
			}
		}
		else if(g_PathName != nullptr)
		{
			std::string pathName;

			pathName += "../";
			pathName += g_PathName;

			for (auto& p : std::filesystem::recursive_directory_iterator(pathName))
			{
				auto e = p.path().string();
				if (e.substr(e.size() - 3, e.size()) == g_AssetFormat)
				{
					g_FileNames.push_back(e);
				}
			}
		}
	}
}
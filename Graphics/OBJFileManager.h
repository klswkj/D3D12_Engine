#pragma once

namespace OBJFileManager
{
	void Initialize(const char* PahtName); // STRONGLY RECOMMENDED
	void SetFormat();
	void FindAllFormatFile();
	void ShutDown();

	extern std::vector<std::string> g_FileNames;
	extern const char* g_PathName;
	extern char g_AssetFormat[4];
}


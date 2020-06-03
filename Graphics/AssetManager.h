#pragma once
#include <vector>
#include <string>
#include <iterator>
#include <filesystem>

class IAssetManager
{
public:
	IAssetManager() = default;
	IAssetManager(const char* pathName);
	virtual ~IAssetManager() = default;
	void Init();

protected:
	virtual void setFormat() = 0;

protected:
	std::vector<std::string> fileNames;
	const char*              PathName;
	char                     AssetFormat[4];
};

IAssetManager::IAssetManager(const char* pathName) 
	: PathName(pathName)
{
}

void IAssetManager::Init()
{
	setFormat();

	fileNames.reserve(std::distance(std::filesystem::directory_iterator(PathName), std::filesystem::directory_iterator{}));

	for (auto& p : std::filesystem::recursive_directory_iterator(PathName))
	{
		auto e = p.path().string();
		if (e.substr(e.size() - 3, e.size()) == AssetFormat)
		{
			fileNames.push_back(e);
		}
	}
}
// °íÃÄ¾ßµÊ.
void IAssetManager::setFormat()
{
	const char* typestring = typeid(this).name();

	for (size_t i{ 0 }; i < 3; ++i)
	{
		AssetFormat[i] = typestring[i] + 32;
	}
	AssetFormat[3] = '\0';
}
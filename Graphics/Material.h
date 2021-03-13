#pragma once
#include "Technique.h"
#include "PSO.h"

struct aiMaterial;
struct aiMesh;

class Material
{
public:
	Material(aiMaterial& material, const std::filesystem::path& path) DEBUG_EXCEPT;

	inline std::vector<Technique> GetTechniques() const noexcept { return m_Techniques; }

private:
	std::vector<Technique> m_Techniques;
	std::string m_ModelFilePath;  // must be std::string
	std::string m_Name;           // must be std::string
};
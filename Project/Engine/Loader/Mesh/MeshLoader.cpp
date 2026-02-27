#include "MeshLoader.h"

#include "Resource/ResourceSystem.h"
#include "Resource/Mesh/MeshManager.h"
#include "Resource/Tex/TextureManager.h"
#include "Utils/Func/UtilFunc.h"
#include "Utils/Logger/Logger.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <filesystem>
#include <limits>
#include <format>

#undef min
#undef max

using namespace Tsumi::Loader;
using namespace tme;
using namespace sys;
using namespace resource;
namespace fs = std::filesystem;

static std::string MakeDefaultDiffuseAlias(const std::string& baseAlias)
{
	return std::format("{}_diffuse", baseAlias);
}

HRESULT MeshLoader::LoadFromScene(const aiScene* scene, const std::string& key, const std::string& alias)
{
	if (!scene || !scene->HasMeshes())
		return E_FAIL;

	if (TryResolveAlias(key, alias))
		return S_OK;

	return RegisterFromScene(scene, key, alias);
}

HRESULT MeshLoader::RegisterFromScene(const aiScene* scene, const std::string& key, const std::string& alias)
{
	auto* meshMgr = ResourceSystem::GetInstance()->GetMeshManager();
	if (!meshMgr) return E_POINTER;

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	HRESULT hr = ParseScene(scene, vertices, indices);
	if (FAILED(hr))
		return hr;

	std::string defaultTexRef = "";
	{
		auto* texMgr = ResourceSystem::GetInstance()->GetTextureManager();
		if (texMgr)
		{
			const std::string defaultAlias = MakeDefaultDiffuseAlias(alias);
			if (texMgr->HasAlias(defaultAlias)) {
				defaultTexRef = defaultAlias; // alias を持つ
			}
		}
	}

	hr = meshMgr->RegisterMesh(key, vertices, indices, defaultTexRef);
	if (FAILED(hr))
		return hr;

	meshMgr->RegisterAlias(alias, key);
	return S_OK;
}

HRESULT MeshLoader::ParseScene(const aiScene* scene, 
							   std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices)
{
	if (!scene || !scene->HasMeshes())
		return E_FAIL;

	outVertices.clear();
	outIndices.clear();

	uint32_t vertexOffset = 0;

	for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
	{
		const aiMesh* mesh = scene->mMeshes[i];
		if (!mesh || !mesh->HasPositions())
			continue;

		// Vertex
		for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
		{
			const aiVector3D& p = mesh->mVertices[v];
			Vertex dst{};

			// position
			dst.pos = { -p.x, p.y, p.z };

			// normal
			if (mesh->HasNormals())
			{
				const aiVector3D& n = mesh->mNormals[v];
				dst.normal = { -n.x, n.y, n.z };
			}
			else
			{
				dst.normal = { 0.0f, 1.0f, 0.0f }; // fallback
			}

			// UV
			if (mesh->HasTextureCoords(0))
			{
				const aiVector3D& uv = mesh->mTextureCoords[0][v];
				dst.uv = { uv.x, uv.y };
			}

			outVertices.push_back(dst);
		}

		// Index
		for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
		{
			const aiFace& face = mesh->mFaces[f];
			if (face.mNumIndices != 3)
				continue;

			outIndices.push_back(vertexOffset + face.mIndices[0]);
			outIndices.push_back(vertexOffset + face.mIndices[2]);
			outIndices.push_back(vertexOffset + face.mIndices[1]);
		}

		vertexOffset += mesh->mNumVertices;
	}

	return (outVertices.empty() || outIndices.empty()) ? E_FAIL : S_OK;
}

bool MeshLoader::TryResolveAlias(const std::string& key, const std::string& alias)
{
	auto* meshMgr = ResourceSystem::GetInstance()->GetMeshManager();
	if (!meshMgr) return false;

	if (meshMgr->HasKey(key)) {
		meshMgr->RegisterAlias(alias, key);
		return true;
	}
	return false;
}

math::Mat4x4 MeshLoader::ToMat4x4(const aiMatrix4x4& a)
{
	return math::Mat4x4(
		a.a1, a.b1, a.c1, a.d1,
		a.a2, a.b2, a.c2, a.d2,
		a.a3, a.b3, a.c3, a.d3,
		a.a4, a.b4, a.c4, a.d4
	);
}

math::Vec3f MeshLoader::TransformPoint(const math::Mat4x4& m, const math::Vec3f& p)
{
	return m.TransformPoint(p);
}

math::Vec3f MeshLoader::TransformNormal(const math::Mat4x4& m, const math::Vec3f& n)
{
	math::Mat4x4 invT = m.Inverse().Transpose();
	return invT.TransformVector(n).Normalized();
}

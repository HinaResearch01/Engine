#include "MeshLoader.h"
#include "Resource/Mesh/MeshManager.h"
#include "Utils/Logger/UtilsLog.h"
#include "Utils/Func/UtilFunc.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>
#include <format>

using namespace Tsumi::Loader;
using namespace Tsumi::Resource;
namespace fs = std::filesystem;

HRESULT MeshLoader::LoadFromFile(const std::string& root, const std::string& name)
{
	if (name.empty()) return E_INVALIDARG;

	// Build path and key
	fs::path p(name);
	fs::path fullPath;
	if (p.is_absolute()) fullPath = p;
	else if (!root.empty()) fullPath = fs::path(root) / p;
	else fullPath = p;

	std::string key = MakeKeyFromRoot(root, name);

	// if already exists, skip
	if (MeshManager::GetInstance()->Has(key)) return S_OK;

	if (!fs::exists(fullPath)) {
		Utils::Log(std::format(L"[MeshLoader] File not found: {}\n", Utils::Utf8ToWstring(fullPath.string())));
		return E_FAIL;
	}

	Assimp::Importer importer;
	const unsigned int flags =
		aiProcess_Triangulate |
		aiProcess_FlipUVs |
		aiProcess_FlipWindingOrder |
		aiProcess_CalcTangentSpace;

	const aiScene* scene = importer.ReadFile(fullPath.string().c_str(), flags);
	if (!scene) {
		Utils::Log(std::format(L"[MeshLoader] Assimp failed: {}\n", Utils::Utf8ToWstring(importer.GetErrorString())));
		return E_FAIL;
	}

	// Let LoadFromScene handle parsing and delegation
	HRESULT hr = LoadFromScene(scene, root, key);
	if (FAILED(hr)) {
		Utils::Log(std::format(L"[MeshLoader] Failed to create mesh from file '{}'\n", Utils::Utf8ToWstring(fullPath.string())));
		return hr;
	}

	return S_OK;
}

HRESULT MeshLoader::LoadFromScene(const aiScene* scene, const std::string& root, const std::string& nameKeyBase)
{
	root;
	if (!scene || !scene->HasMeshes()) return S_OK;

	HRESULT overall = S_OK;
	overall;

	// We'll consolidate all meshes into a single mesh as previous behavior did.
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	vertices.reserve(10240);
	indices.reserve(10240);
	uint32_t vertexOffset = 0;

	for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi) {
		const aiMesh* aimesh = scene->mMeshes[mi];
		std::vector<Vertex> localV;
		std::vector<uint32_t> localI;

		if (!ParseAiMesh(aimesh, localV, localI))
			continue;

		vertices.insert(vertices.end(), localV.begin(), localV.end());
		for (uint32_t idx : localI) indices.push_back(vertexOffset + idx);
		vertexOffset = static_cast<uint32_t>(vertices.size());
	}

	if (vertices.empty() || indices.empty()) {
		Utils::Log(std::format(L"[MeshLoader] No valid geometry in scene '{}'\n", Utils::Utf8ToWstring(nameKeyBase)));
		return E_FAIL;
	}

	// Delegate GPU upload & storage to MeshManager
	return MeshManager::GetInstance()->CreateFromCpuData(nameKeyBase, vertices, indices);
}

bool MeshLoader::ParseAiMesh(const aiMesh* src, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices)
{
	if (!src || !src->HasPositions())
		return false;

	outVertices.resize(src->mNumVertices);
	outIndices.clear();

	// --- vertex conversion (RH -> LH) ---
	for (unsigned int i = 0; i < src->mNumVertices; ++i) {
		const aiVector3D& p = src->mVertices[i];
		Vertex v{};

		v.pos.x = -p.x;   // X flip RH->LH
		v.pos.y = p.y;
		v.pos.z = p.z;

		// normal
		if (src->HasNormals()) {
			const aiVector3D& n = src->mNormals[i];
			v.normal.x = -n.x;
			v.normal.y = n.y;
			v.normal.z = n.z;
		}

		// uv
		if (src->HasTextureCoords(0)) {
			const aiVector3D& uv = src->mTextureCoords[0][i];
			v.uv.x = uv.x;
			v.uv.y = uv.y;
		}

		outVertices[i] = v;
	}

	// indices (triangles)
	for (unsigned int f = 0; f < src->mNumFaces; ++f) {
		const aiFace& face = src->mFaces[f];
		if (face.mNumIndices == 3) {
			outIndices.push_back(face.mIndices[0]);
			outIndices.push_back(face.mIndices[1]);
			outIndices.push_back(face.mIndices[2]);
		}
	}

	return !(outVertices.empty() || outIndices.empty());
}

std::string MeshLoader::MakeKeyFromRoot(const std::string& root, const std::string& name)
{
	fs::path p(name);
	fs::path full;
	if (p.is_absolute()) {
		if (!root.empty()) {
			try {
				fs::path rootp(root);
				fs::path rel = fs::relative(p, rootp);
				if (!rel.empty()) {
					return (rootp / rel).lexically_normal().string();
				}
			}
			catch (...) { /* fallback to absolute */ }
		}
		return p.lexically_normal().string();
	}
	else {
		if (!root.empty()) full = fs::path(root) / p;
		else full = p;
		return full.lexically_normal().string();
	}
}
module;
#include <filesystem>
#include <stdexcept>
#include <cassert>
#include <algorithm>
#include <span>
#include <format>
#include <memory>
#include <optional>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

module Brawler.LODResolver;
import Brawler.ImportedMesh;
import Util.ModelExport;
import Brawler.LaunchParams;
import Brawler.LODScene;
import Util.Win32;
import Util.General;
import Brawler.MeshTypeID;
import Brawler.StaticMeshResolver;

namespace
{
	Brawler::MeshTypeID GetMeshTypeID(const aiMesh& mesh)
	{
		// TODO: Add support for skinned meshes. For now, we assert that the mesh is not
		// skinned and simply create StaticMeshResolvers.

		if (mesh.HasBones()) [[unlikely]]
			throw std::runtime_error{ std::string{ "ERROR: The mesh " } + mesh.mName.C_Str() + " is a skinned mesh. Skinned meshes are currently unsupported." };

		return Brawler::MeshTypeID::STATIC;
	}
}

namespace Brawler
{
	LODResolver::LODResolver(const std::uint32_t lodLevel) :
		mImporter(),
		mAIScenePtr(nullptr),
		mMeshResolverCollectionPtr(nullptr),
		mLODLevel(lodLevel)
	{}

	void LODResolver::ImportScene()
	{
		CreateAIScene();
		CreateMeshResolvers();

		// Notify the user that the LOD mesh represented by this LODResolver has been imported.
		const std::filesystem::path& lodMeshFilePath{ Util::ModelExport::GetLaunchParameters().GetLODFilePath(mLODLevel) };
		Util::Win32::WriteFormattedConsoleMessage(std::format(L"LOD {} Mesh Import Finished (Mesh File: {})", mLODLevel, lodMeshFilePath.c_str()));
	}

	void LODResolver::Update()
	{
		mMeshResolverCollectionPtr->Update();
	}

	bool LODResolver::IsReadyForSerialization() const
	{
		return mMeshResolverCollectionPtr->IsReadyForSerialization();
	}

	const aiScene& LODResolver::GetScene() const
	{
		assert(mAIScenePtr != nullptr);
		return *mAIScenePtr;
	}

	std::uint32_t LODResolver::GetLODLevel() const
	{
		return mLODLevel;
	}

	void LODResolver::CreateAIScene()
	{
		const std::filesystem::path& fbxFile{ Util::ModelExport::GetLaunchParameters().GetLODFilePath(mLODLevel) };
		
		// Check that the provided file is an FBX file. Although Assimp supports files of arbitrary
		// types, there are certain properties of FBX files which we assume throughout the import
		// process. For instance, FBX's unit system is 1 unit = 1 centimeter, but the Brawler Engine uses
		// 1 unit = 1 meter.
		//
		// We only check for file extension, rather than for the magic header of the file, since this
		// program is just a tool for our use.
		{
			const std::filesystem::path fileExtension{ fbxFile.extension() };
			
			std::string fileExtStr{ fileExtension.string() };
			std::ranges::transform(fileExtStr, fileExtStr.begin(), [] (const char c) { return std::tolower(c); });

			if (fileExtStr != ".fbx") [[unlikely]]
				throw std::runtime_error{ std::string{"ERROR: The model file "} + fbxFile.string() + " is not a valid .FBX file!" };
		}
		
		mImporter.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType::aiPrimitiveType_LINE | aiPrimitiveType::aiPrimitiveType_POINT);
		mImporter.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 0.01f);

		mAIScenePtr = mImporter.ReadFile(
			fbxFile.string(),
			aiProcessPreset_TargetRealtime_MaxQuality |
			aiProcess_ConvertToLeftHanded |
			aiPostProcessSteps::aiProcess_TransformUVCoords |
			aiPostProcessSteps::aiProcess_GlobalScale |
			aiPostProcessSteps::aiProcess_OptimizeGraph
		);

		if (mAIScenePtr == nullptr) [[unlikely]]
			throw std::runtime_error{ std::string{ "ERROR: The model file " } + fbxFile.string() + " could not be imported!" };
	}

	void LODResolver::CreateMeshResolvers()
	{
		const std::span<const aiMesh*> meshSpan{ const_cast<const aiMesh**>(mAIScenePtr->mMeshes), mAIScenePtr->mNumMeshes };

		// Dynamically determine the type of MeshResolverCollection which we will use.
		std::optional<MeshTypeID> lodMeshTypeID{};

		for (const auto meshPtr : meshSpan)
		{
			const MeshTypeID currTypeID = GetMeshTypeID(*meshPtr);

			if (!lodMeshTypeID.has_value()) [[unlikely]]
				lodMeshTypeID = currTypeID;

			else if (*lodMeshTypeID != currTypeID) [[unlikely]]
				throw std::runtime_error{ Util::General::WStringToString(std::format(LR"(ERROR: Multiple mesh types were detected in the LOD mesh file "{}!")", Util::ModelExport::GetLaunchParameters().GetLODFilePath(mLODLevel).c_str())) };
		}

		switch (*lodMeshTypeID)
		{
		case MeshTypeID::STATIC:
		{
			mMeshResolverCollectionPtr = std::make_unique<MeshResolverCollection<StaticMeshResolver>>();
			break;
		}

		// TODO: Add support for skinned meshes. For now, we assert that the mesh is not
		// skinned and simply create StaticMeshResolvers.

		case MeshTypeID::SKINNED: [[fallthrough]];
		default:
		{
			assert(false);
			std::unreachable();

			return;
		}
		}

		// Have the MeshResolverCollection create a mesh resolver for each aiMesh which we
		// imported.
		for (const auto meshPtr : meshSpan)
			mMeshResolverCollectionPtr->CreateMeshResolverForImportedMesh(ImportedMesh{ *meshPtr, LODScene{*mAIScenePtr, GetLODLevel()} });
	}
}
#pragma once

#include "NodeAttribute.h"
#include "IMaterialContainer.h"

namespace ve {

	class IModel : public NodeAttribute, public IMaterialContainer
	{
	public:
		virtual ~IModel() {}

		virtual bool Load(LoggerPtr logger, ModelSourcePtr source, const ModelRendererConfig& config) = 0;
		virtual bool Load(LoggerPtr logger, const wchar_t* pFilePath) = 0;
		virtual bool Save(LoggerPtr logger) = 0;

		virtual const wchar_t* GetFilePath() const = 0;

		virtual NodePtr GetRootNode() = 0;
		virtual uint32_t GetNodeCount() const = 0;
		virtual NodePtr GetNode(uint32_t nodeIndex) = 0;

		virtual uint32_t GetMeshCount() const = 0;
		virtual MeshPtr GetMesh(uint32_t meshIndex) = 0;

		virtual uint32_t GetPolygonCount() const = 0;
	};

}
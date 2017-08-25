#pragma once

#include "IModelSource.h"
#include "Scene.h"
#include "NodeAttribute.h"
#include "XmlUtility.h"

namespace ve {

	class Project
	{
	public:
		static constexpr wchar_t* SceneModelName = L"Model";
		static constexpr uint32_t SceneModelGroup = 0x00000001;

		static ProjectPtr Create();

		Project();
		~Project();

		void New(const wchar_t* pFilePath);
		bool Open(LoggerPtr logger, DeviceContextPtr deviceContext, ScenePtr scene, const wchar_t* pFilePath);
		bool Save(LoggerPtr logger, ScenePtr scene);

		const wchar_t* GetFilePathWithoutExtension() const;

		VE_DECLARE_ALLOCATOR

	private:
		static constexpr wchar_t* V3DEditorVersion = L"1.0.0";
		static constexpr wchar_t* SceneVersion = L"1.0.0";
		static constexpr wchar_t* CameraVersion = L"1.0.0";
		static constexpr wchar_t* ModelVersion = L"1.0.0";

		static constexpr wchar_t* XmlV3DEditorVersion = L"1.0.0";
		static constexpr wchar_t* XmlSceneVersion = L"1.0.0";

		static constexpr wchar_t* XmlDisplayNames[] =
		{
			L"lightShape",
			L"meshBounds",
		};

		static constexpr uint32_t XmlDisplayDrawFlags[] =
		{
			DEBUG_DRAW_LIGHT_SHAPE,
			DEBUG_DRAW_MESH_BOUNDS,
		};

		static constexpr DEBUG_COLOR_TYPE XmlDisplayColorTypes[] =
		{
			DEBUG_COLOR_TYPE_LIGHT_SHAPE,
			DEBUG_COLOR_TYPE_MESH_BOUNDS,
		};

		static auto constexpr XmlDisplayCount = 2;

		struct XmlAttribute
		{
			const wchar_t* pName;
			const wchar_t* pValue;
		};

		StringW m_FilePath;
		StringW m_DirPath;

		bool LoadRoot(xml::Element* pElement, LoggerPtr logger, DeviceContextPtr deviceContext, ScenePtr scene);
		bool LoadScene(xml::Element* pElement, LoggerPtr logger, DeviceContextPtr deviceContext, ScenePtr scene);
		bool LoadGrid(xml::Element* pElement, ScenePtr scene);
		bool LoadSsao(xml::Element* pElement, ScenePtr scene);
		bool LoadShadow(xml::Element* pElement, ScenePtr scene);
		bool LoadGlare(xml::Element* pElement, ScenePtr scene);
		bool LoadImageEffect(xml::Element* pElement, ScenePtr scene);
		bool LoadMisc(xml::Element* pElement, ScenePtr scene);
		bool LoadNode(xml::Element* pElement, LoggerPtr logger, DeviceContextPtr deviceContext, ScenePtr scene, NodePtr node);
		NodePtr LoadAttribute(xml::Element* pElemen, ScenePtr scene, NodePtr parent);
		NodePtr LoadCamera(xml::Element* pElemen, ScenePtr scene);
		NodePtr LoadLight(xml::Element* pElemen, ScenePtr scene);
		NodePtr LoadModel(xml::Element* pElemen, LoggerPtr logger, DeviceContextPtr deviceContext, ScenePtr scene, NodePtr parent);

		bool SaveRoot(xml::Element* pDocument, LoggerPtr logger, ScenePtr scene);
		bool SaveScene(xml::Element* pParent, LoggerPtr logger, ScenePtr scene);
		bool SaveGrid(xml::Element* pParent, const Scene::Grid& grid);
		bool SaveSsao(xml::Element* pParent, const Scene::Ssao& ssao);
		bool SaveShadow(xml::Element* pParent, const Scene::Shadow& shadow);
		bool SaveGlare(xml::Element* pParent, const Scene::Glare& glare);
		bool SaveImageEffect(xml::Element* pParent, const Scene::ImageEffect& imageEffect);
		bool SaveMisc(xml::Element* pParent, ScenePtr scene);
		bool SaveNode(xml::Element* pParent, LoggerPtr logger, const NodePtr node);
		bool SaveAttribute(xml::Element* pElement, NodeAttribute::TYPE type);
		bool SaveCamera(xml::Element* pElement, CameraPtr camera);
		bool SaveLight(xml::Element* pElement, LightPtr light);
		bool SaveModel(xml::Element* pElement, LoggerPtr logger, ModelPtr model);

		static bool ParseColor(const StringW& value, glm::vec4& color);
	};

}
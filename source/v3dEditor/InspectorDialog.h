#pragma once

#include "GuiFloat.h"
#include "FileBrowser.h"

namespace ve {

	class InspectorDialog final : public GuiFloat
	{
	public:
		InspectorDialog();
		virtual ~InspectorDialog();

		void SetDeviceContext(DeviceContextPtr deviceContext);

		void SetScene(ScenePtr scene);
		void SetNode(NodePtr node);

		void Dispose();

	protected:
		bool OnRender() override;

	private:
		static constexpr char* TextureFilters[] =
		{
			"Nearest",
			"Linear",
		};

		static constexpr char* TextureAddressModes[] =
		{
			"Repeat",
			"MirroredRepeat",
			"ClampToEdge",
		};

		static constexpr char* BlendModes[] =
		{
			"Copy",
			"Normal",
			"Add",
			"Sub",
			"Mul",
			"Screen",
		};

		static constexpr char* CullModes[] =
		{
			"None",
			"Front",
			"Back",
		};

		DeviceContextPtr m_DeviceContext;

		ScenePtr m_Scene;
		NodePtr m_StartNode;
		NodePtr m_Node;
		NodeAttributePtr m_NodeAttribute;

		StringA m_PolygonCount;
		StringA m_MeshCount;
		StringA m_BoneCount;

		collection::Vector<StringA> m_MaterialNameList;
		collection::Vector<const char*> m_MaterialList;
		StringA m_DiffuseTexture;
		FileBrowser m_DiffuseTextureBrowser;
		StringA m_SpecularTexture;
		FileBrowser m_SpecularTextureBrowser;
		StringA m_BumpTexture;
		FileBrowser m_BumpTextureBrowser;
		int32_t m_TextureFilter;
		int32_t m_TextureAddressMode;
		int32_t m_BlendMode;
		int32_t m_CullMode;

		void RenderScene();
		void RenderCamera();
		void RenderLight();
		void RenderModel();
		void RenderMesh();

		void RenderMaterialContainer(MaterialContainerPtr materialContainer);
	};

}
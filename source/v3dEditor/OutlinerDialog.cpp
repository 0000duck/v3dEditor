#include "OutlinerDialog.h"
#include "Scene.h"
#include "IModel.h"
#include "IMesh.h"
#include "Node.h"

namespace ve {

	OutlinerDialog::OutlinerDialog() : GuiFloat("Outliner", glm::ivec2(384, 256))
	{
	}

	OutlinerDialog::~OutlinerDialog()
	{
	}

	NodePtr OutlinerDialog::GetNode()
	{
		return m_Node;
	}

	void OutlinerDialog::SetScene(ScenePtr scene)
	{
		m_Scene = scene;
		m_Node = m_Scene->GetRootNode();
	}

	void OutlinerDialog::SetNode(NodePtr node)
	{
		if (node != nullptr)
		{
			m_Node = nullptr;
			SelectNode(m_Scene->GetRootNode(), node);
		}
		else
		{
			m_Node = m_Scene->GetRootNode();
		}
	}

	void OutlinerDialog::Dispose()
	{
		m_Node = nullptr;
		m_Scene = nullptr;
	}

	void OutlinerDialog::RenderNode(NodePtr node)
	{
		uint32_t childCount = node->GetChildCount();
		NodeAttributePtr attribute = node->GetAttribute();

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnDoubleClick;
		if (node == m_Node) { flags |= ImGuiTreeNodeFlags_Selected; }
		if ((attribute->GetType() != NodeAttribute::TYPE_MODEL) && (childCount == 0)) { flags |= ImGuiTreeNodeFlags_Leaf; }

		bool invisible = false;
		if (attribute->GetType() == NodeAttribute::TYPE_MESH)
		{
			MeshPtr mesh = std::static_pointer_cast<IMesh>(attribute);
			if (mesh->GetVisible() == false)
			{
				invisible = true;
			}
		}

		if (invisible == true)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
		}

		if (ImGui::TreeNodeEx(node->GetNameA(), flags) == true)
		{
			if ((ImGui::IsItemHovered() == true) && (ImGui::IsMouseClicked(0) == true))
			{
				if (m_Node != node)
				{
					m_Node = node;
					SetResult(OutlinerDialog::RESULT_SELECT_CHANGED);
				}
			}

			if (attribute->GetType() == NodeAttribute::TYPE_MODEL)
			{
				ModelPtr model = std::static_pointer_cast<IModel>(attribute);
				RenderNode(model->GetRootNode());
			}

			for (uint32_t i = 0; i < childCount; i++)
			{
				RenderNode(node->GetChild(i));
			}

			ImGui::TreePop();

			if (invisible == true)
			{
				ImGui::PopStyleVar();
			}
		}
	}

	void OutlinerDialog::SelectNode(NodePtr node, NodePtr compNode)
	{
		uint32_t childCount = node->GetChildCount();

		if (node == compNode)
		{
			m_Node = node;
			return;
		}

		NodeAttributePtr attribute = node->GetAttribute();
		if (attribute->GetType() == NodeAttribute::TYPE_MODEL)
		{
			ModelPtr model = std::static_pointer_cast<IModel>(attribute);
			SelectNode(model->GetRootNode(), compNode);
		}

		for (uint32_t i = 0; i < childCount; i++)
		{
			SelectNode(node->GetChild(i), compNode);
		}
	}

	bool OutlinerDialog::OnRender()
	{
		SetResult(0);

		RenderNode(m_Scene->GetRootNode());

		return false;
	}

}
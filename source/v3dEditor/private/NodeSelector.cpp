#include "NodeSelector.h"
#include "Node.h"
#include "NodeAttribute.h"
#include "IModel.h"

namespace ve {

	NodeSelector* NodeSelector::Create()
	{
		return VE_NEW_T(NodeSelector);
	}

	NodeSelector::NodeSelector() :
		m_Index(1)
	{
	}

	NodeSelector::~NodeSelector()
	{
	}

	void NodeSelector::Destroy()
	{
		VE_DELETE_THIS_T(this, NodeSelector);
	}

	void NodeSelector::Clear()
	{
		if (m_Map.empty() == false)
		{
			auto it_begin = m_Map.begin();
			auto it_end = m_Map.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				m_UnusedKeys.push_back(it->first);
			}

			m_Map.clear();
		}

		m_RemoveMap.clear();

		m_Found = nullptr;
	}

	void NodeSelector::Add(NodePtr node)
	{
		NodeAttributePtr attribute = node->GetAttribute();
		if (attribute->IsSelectSupported() == true)
		{
			uint32_t key = 0;

			if (m_UnusedKeys.empty() == false)
			{
				key = m_UnusedKeys.back();
				m_UnusedKeys.pop_back();
			}
			else
			{
				key = m_Index++;
			}

			m_Map[key] = node;
			m_RemoveMap[node] = m_Map.find(key);

			attribute->SetSelectKey(key);
		}

		if (attribute->GetType() == NodeAttribute::TYPE_MODEL)
		{
			ModelPtr model = std::static_pointer_cast<IModel>(attribute);
			Add(model->GetRootNode());
		}

		uint32_t childCount = node->GetChildCount();
		for (uint32_t i = 0; i < childCount; i++)
		{
			Add(node->GetChild(i));
		}
	}

	void NodeSelector::Remove(NodePtr node)
	{
		auto it = m_RemoveMap.find(node);
		if (it == m_RemoveMap.end())
		{
			return;
		}

		if (it->first == m_Found)
		{
			m_Found = nullptr;
		}

		NodeAttributePtr attribute = node->GetAttribute();
		if (attribute->IsSelectSupported() == true)
		{
			m_UnusedKeys.push_back(it->second->first);

			m_Map.erase(it->second);
			m_RemoveMap.erase(it);

			attribute->SetSelectKey(0);
		}

		if (attribute->GetType() == NodeAttribute::TYPE_MODEL)
		{
			ModelPtr model = std::static_pointer_cast<IModel>(attribute);
			Add(model->GetRootNode());
		}

		uint32_t childCount = node->GetChildCount();
		for (uint32_t i = 0; i < childCount; i++)
		{
			Remove(node->GetChild(i));
		}
	}

	NodePtr NodeSelector::Find(uint32_t key)
	{
		auto it_mesh = m_Map.find(key);
		if (it_mesh == m_Map.end())
		{
			m_Found = nullptr;
			return nullptr;
		}

		m_Found = it_mesh->second;

		return m_Found;
	}

	NodePtr NodeSelector::GetFound()
	{
		return m_Found;
	}

	void NodeSelector::SetFound(NodePtr attribute)
	{
		m_Found = attribute;
	}

}
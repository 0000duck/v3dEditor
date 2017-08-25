#pragma once

#include "GuiFloat.h"

namespace ve {

	class OutlinerDialog final : public GuiFloat
	{
	public:
		enum RESULT
		{
			RESULT_SELECT_CHANGED = 1,
		};

		OutlinerDialog();
		virtual~OutlinerDialog();

		void SetScene(ScenePtr scene);

		NodePtr GetNode();
		void SetNode(NodePtr node);

		void Dispose();

	private:
		ScenePtr m_Scene;
		NodePtr m_Node;

		void RenderNode(NodePtr node);
		void SelectNode(NodePtr node, NodePtr compNode);

		bool OnRender() override;
	};

}
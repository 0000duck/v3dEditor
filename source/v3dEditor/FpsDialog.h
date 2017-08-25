#pragma once

#include "GuiFloat.h"

namespace ve {

	class FpsDialog final : public GuiFloat
	{
	public:
		FpsDialog();
		virtual~FpsDialog();

		void Render(const glm::uvec2& screenSize, float fps, float elapsedTime);

	private:
		glm::uvec2 m_ScreenSize;
		float m_Fps;
		float m_ElapsedTime;

		bool OnRender() override;
	};

}
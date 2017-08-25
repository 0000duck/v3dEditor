#pragma once

namespace ve {

	class GuiWindow
	{
	public:
		GuiWindow(const char* pHash, ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders);
		GuiWindow(const char* pHash, const glm::ivec2& size, ImGuiWindowFlags flags = ImGuiWindowFlags_ShowBorders);
		GuiWindow(const char* pHash, const glm::ivec2& pos, const glm::ivec2& size, ImGuiWindowFlags flags = ImGuiWindowFlags_ShowBorders);
		virtual ~GuiWindow();

		const char* GetName() const;

		const char* GetCaption() const;
		void SetCaption(const char* pCaption);

		const char* GetHash() const;
		void SetHash(const char* pHash);

		const glm::ivec2& GetPosition() const;
		const glm::ivec2& GetSize() const;

		ImGuiWindowFlags GetInternalFlags() const;

		int32_t GetResult() const;
		void SetResult(int32_t result);

		virtual bool IsShow() const = 0;
		virtual void Show() = 0;

		virtual int32_t Render();

	protected:
		enum STATE
		{
			STATE_IDLING = 0,
			STATE_SHOW = 1,
			STATE_HIDE = 2,
		};

		virtual GuiWindow::STATE OnBegin() = 0;
		virtual void OnEnd() = 0;
		virtual void OnClose() = 0;
		virtual bool OnRender() = 0;

	private:
		StringA m_Caption;
		StringA m_Hash;
		StringA m_Name;
		glm::ivec2 m_Pos;
		bool m_PosEnable;
		glm::ivec2 m_Size;
		ImGuiWindowFlags m_Flags;
		int32_t m_Result;
	};

}
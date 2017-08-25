#pragma once

#include "GuiPopup.h"
#include "FileBrowser.h"

namespace ve {

	class ImportDialog final : public GuiPopup
	{
	public:
		enum RESULT
		{
			RESULT_OK = 1,
			RESULT_CANCEL = 2,
		};

		struct Data
		{
			StringW sourceFilePath;
			ModelSourceConfig sourceConfig;
			ModelRendererConfig rednererConfig;
		};

		ImportDialog();
		virtual~ImportDialog();

		const ImportDialog::Data& GetData() const;

	private:
		StringA m_FilePath;
		bool m_FlipFaceEnable;
		bool m_InvertTexCoordUEnable;
		bool m_InvertTexCoordVEnable;
		bool m_InvertNormalEnable;
		bool m_TransformEnable;
		float m_Rotate[3];
		float m_Scale;
		bool m_OptimizeEnable;
		bool m_SommosingEnable;
		float m_SmoosingAngle;
		int32_t m_PathType;

		Data m_Data;

		FileBrowser m_FileBrowser;

		bool OnRender() override;
		void OnClose() override;
	};

}
#pragma once

#include "GuiPopup.h"

namespace ve {

	class MessageDialog final : public GuiPopup
	{
	public:
		enum RESULT
		{
			RESULT_OK = 1,
			RESULT_CANCEL = 2,
			RESULT_RETRY = 3,
			RESULT_YES = 4,
			RESULT_NO = 5,
		};

		enum TYPE
		{
			TYPE_OK = 0,
			TYPE_OK_CANCEL = 1,
			TYPE_RETRY_CANCEL = 2,
			TYPE_YES_NO = 3,
			TYPE_YES_NO_CANCEL = 4,
		};

		MessageDialog();
		virtual~MessageDialog();

		MessageDialog::TYPE GetType() const;
		void SetType(MessageDialog::TYPE type);

		const char* GetText() const;
		void SetText(const char* pMessage);

		int32_t GetUserIndex() const;
		void SetUserIndex(int32_t index);

	private:
		MessageDialog::TYPE m_Type;
		StringA m_Text;
		int32_t m_UserIndex;

		bool OnRender() override;
	};

}
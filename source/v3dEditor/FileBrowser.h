#pragma once

#include "GuiPopup.h"

namespace ve {

	class FileBrowser final : public GuiPopup
	{
	public:
		enum RESULT
		{
			RESULT_OK = 1,
			RESULT_CANCEL = 2,
		};

		enum MODE
		{
			MODE_OPEN = 0,
			MODE_SAVE = 1,
		};

		FileBrowser();
		virtual ~FileBrowser();

		FileBrowser::MODE GetMode() const;
		void SetMode(FileBrowser::MODE mode);

		void ClearExtensions();
		uint32_t GetExtensionCount() const;
		const char* GetGetExtension(uint32_t extesionIndex) const;
		void AddExtension(const char* pExtension);

		const char* GetFilePath() const;
		void SetFilePath(const char* pFilePath);

	protected:
		bool OnRender() override;

	private:
		struct DriveInfo
		{
			StringA name;
			StringA displayName;
		};

		struct ObjectInfo
		{
			bool directory;
			StringA name;

			bool operator < (const ObjectInfo& rhs) const
			{
				if (directory > rhs.directory)
				{
					return true;
				}
				else if (directory < rhs.directory)
				{
					return false;
				}

				if (name < rhs.name)
				{
					return true;
				}
				else if (name > rhs.name)
				{
					return false;
				}

				return false;
			}
		};

		FileBrowser::MODE m_Mode;

		collection::Vector<StringA> m_Extensions;

		collection::Vector<DriveInfo> m_DriveInfos;
		collection::Vector<const char*> m_Drives;
		int32_t m_DriveIndex;

		StringA m_Directory;

		collection::Vector<ObjectInfo> m_ObjectInfos;
		collection::Vector<const char*> m_Objects;
		int32_t m_ObjectIndex;

		char m_TempFileName[_MAX_PATH];
		StringA m_FileName;
		StringA m_FilePath;

		void SetDirectory(const char* pPath);
		void UpDirectory();
		void DownDirectory(const char* pPath);
	};

}
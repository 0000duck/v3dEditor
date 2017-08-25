#include "FileBrowser.h"
#include<Shlwapi.h>

namespace ve {

	FileBrowser::FileBrowser() : GuiPopup("", true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders),
		m_Mode(FileBrowser::MODE_OPEN),
		m_DriveIndex(0),
		m_ObjectIndex(0)
	{
		char curDir[_MAX_PATH]{};
		GetCurrentDirectoryA(_MAX_PATH, curDir);

		uint32_t driveFlags = GetLogicalDrives();

		for (uint32_t i = 0; i < 26; i++)
		{
			if (driveFlags & (1 << i))
			{
				char drive = ('A' + i);
				char driveName[4];
				sprintf_s(driveName, "%c:\\", drive);

				char volumeName[_MAX_PATH]{};
				char volumeSystem[_MAX_PATH]{};
				DWORD volumeLength = 0;
				DWORD volumeFlags = 0;

				GetVolumeInformationA(driveName, volumeName, _countof(volumeName), nullptr, &volumeLength, &volumeFlags, volumeSystem, _countof(volumeSystem));

				DriveInfo driveInfo;
				driveInfo.name = driveName;

				if (strlen(volumeName) > 0)
				{
					driveInfo.displayName = volumeName;
					driveInfo.displayName += " (";
					driveInfo.displayName += drive;
					driveInfo.displayName += ":)";
				}
				else
				{
					driveInfo.displayName = "(";
					driveInfo.displayName += drive;
					driveInfo.displayName += ":)";
				}

				m_DriveInfos.push_back(driveInfo);

				if (curDir[0] == drive)
				{
					m_DriveIndex = static_cast<int32_t>(m_DriveInfos.size() - 1);
				}
			}

			memset(m_TempFileName, 0, sizeof(m_TempFileName));
		}

		if (m_DriveInfos.empty() == false)
		{
			m_Drives.reserve(m_DriveInfos.size());

			auto it_begin = m_DriveInfos.begin();
			auto it_end = m_DriveInfos.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				m_Drives.push_back(it->displayName.c_str());
			}
		}

		SetDirectory(curDir);
	}

	FileBrowser::~FileBrowser()
	{
	}

	FileBrowser::MODE FileBrowser::GetMode() const
	{
		return m_Mode;
	}

	void FileBrowser::SetMode(FileBrowser::MODE mode)
	{
		m_Mode = mode;
	}

	void FileBrowser::ClearExtensions()
	{
		m_Extensions.clear();
	}

	uint32_t FileBrowser::GetExtensionCount() const
	{
		return static_cast<uint32_t>(m_Extensions.size());
	}

	const char* FileBrowser::GetGetExtension(uint32_t extesionIndex) const
	{
		return m_Extensions[extesionIndex].c_str();
	}

	void FileBrowser::AddExtension(const char* pExtension)
	{
		StringA temp = pExtension;
		std::transform(temp.begin(), temp.end(), temp.begin(), ::tolower);

		m_Extensions.push_back(temp);
	}

	const char* FileBrowser::GetFilePath() const
	{
		return m_FilePath.c_str();
	}

	void FileBrowser::SetFilePath(const char* pFilePath)
	{
		size_t length = strlen(pFilePath);
		if (length == 0)
		{
			return;
		}

		StringA absolutePath;

		if ((length > 3) && (pFilePath[1] == ':'))
		{
			absolutePath = pFilePath;
		}
		else
		{
			char currentDirectory[_MAX_PATH];
			GetCurrentDirectoryA(_MAX_PATH, currentDirectory);

			absolutePath = currentDirectory;

			if ((absolutePath.back() != '\\') && (absolutePath.back() != '/') && (pFilePath[0] != '\\') && (pFilePath[0] != '/'))
			{
				absolutePath += '\\';
			}

			absolutePath += pFilePath;
		}

		if ((absolutePath.back() == '\\') || (absolutePath.back() == '/'))
		{
			SetDirectory(absolutePath.c_str());
			m_FilePath = "";
			m_FileName = "";
		}
		else
		{
			StringA tempPath = absolutePath;
			tempPath += "\\*.*";

			WIN32_FIND_DATAA wfd;
			HANDLE findHandle = FindFirstFileA(tempPath.c_str(), &wfd);
			if (findHandle == INVALID_HANDLE_VALUE)
			{
				int32_t separatePoint = 0;
				for (separatePoint = static_cast<int32_t>(absolutePath.size() - 1); separatePoint >= 0; separatePoint--)
				{
					if ((absolutePath[separatePoint] == '\\') || (absolutePath[separatePoint] == '/'))
					{
						break;
					}
				}

				if (separatePoint >= 2)
				{
					StringA directoryPath = absolutePath.substr(0, separatePoint + 1);
					SetDirectory(directoryPath.c_str());

					m_FilePath = absolutePath;
					m_FileName = absolutePath.substr(separatePoint + 1);
				}
				else
				{
					absolutePath += "\\";

					SetDirectory(absolutePath.c_str());
					m_FilePath = "";
					m_FileName = "";
				}
			}
			else
			{
				FindClose(findHandle);

				absolutePath += "\\";

				SetDirectory(absolutePath.c_str());
				m_FilePath = "";
				m_FileName = "";
			}
		}
	}

	bool FileBrowser::OnRender()
	{
		bool selectFile = false;

		SetResult(FileBrowser::RESULT_CANCEL);

		// ----------------------------------------------------------------------------------------------------
		// ドライブリスト
		// ----------------------------------------------------------------------------------------------------

		if (ImGui::ListBox("##FileBrowser_DriveList", &m_DriveIndex, m_Drives.data(), static_cast<int32_t>(m_Drives.size())) == true)
		{
			SetDirectory(m_DriveInfos[m_DriveIndex].name.c_str());
		}

		ImGui::Spacing();

		// ----------------------------------------------------------------------------------------------------
		// カレントディレクトリ
		// ----------------------------------------------------------------------------------------------------

		ImGui::InputText("##FileBrowser_DirectoryPath", &m_Directory[0], static_cast<int32_t>(m_Directory.size()), ImGuiInputTextFlags_ReadOnly);

		ImGui::SameLine();
		if (ImGui::Button("<###FileBrowser_Directory_Up") == true)
		{
			UpDirectory();
		}

		ImGui::SameLine();
		if (ImGui::Button(">###FileBrowser_Directory_Down") == true)
		{
			if (m_ObjectInfos.empty() == false)
			{
				const FileBrowser::ObjectInfo& info = m_ObjectInfos[m_ObjectIndex];
				DownDirectory(info.name.c_str());
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// ファイルリスト
		// ----------------------------------------------------------------------------------------------------

		if (ImGui::ListBox("##FileBrowser_List", &m_ObjectIndex, m_Objects.data(), static_cast<int32_t>(m_Objects.size())) == true)
		{
			if (m_ObjectInfos[m_ObjectIndex].directory == false)
			{
				m_FileName = m_ObjectInfos[m_ObjectIndex].name;
			}
		}

		if ((ImGui::IsItemHovered() == true) &&
			(ImGui::IsMouseDoubleClicked(0) == true) &&
			(m_ObjectInfos.size() > 0))
		{
			const FileBrowser::ObjectInfo& info = m_ObjectInfos[m_ObjectIndex];

			if (info.directory == true)
			{
				// ディレクトリを移動
				if (strcmp(info.name.c_str(), "..") == 0)
				{
					// 上る
					UpDirectory();
				}
				else
				{
					// 下る
					DownDirectory(info.name.c_str());
				}
			}
			else
			{
				// ファイルが選択された
				selectFile = true;
			}
		}

		ImGui::Spacing();

		// ----------------------------------------------------------------------------------------------------
		// ファイル名
		// ----------------------------------------------------------------------------------------------------

		char fileButtonName[32]{};

		switch (m_Mode)
		{
		case FileBrowser::MODE_OPEN:
			strcpy_s(fileButtonName, "Open##FileBrowser_File_Open");
			break;
		case FileBrowser::MODE_SAVE:
			strcpy_s(fileButtonName, "Save##FileBrowser_File_Save");
			break;
		}

		strcpy_s(m_TempFileName, m_FileName.c_str());
		if (ImGui::InputText("##FileBrowser_FileName", m_TempFileName, sizeof(m_TempFileName)) == true)
		{
			m_FileName = m_TempFileName;
		}
		ImGui::SameLine();

		if (ImGui::Button(fileButtonName) == true)
		{
			selectFile = true;
		}

		// ----------------------------------------------------------------------------------------------------
		// ファイルの選択処理
		// ----------------------------------------------------------------------------------------------------

		bool result = false;

		char fileErrorMessage[32]{};

		switch (m_Mode)
		{
		case FileBrowser::MODE_OPEN:
			strcpy_s(fileErrorMessage, "File not found");
			break;
		case FileBrowser::MODE_SAVE:
			strcpy_s(fileErrorMessage, "Directory not found");
			break;
		}

		if (selectFile == true)
		{
			StringA filePath = m_Directory + m_FileName;

			switch (m_Mode)
			{
			case FileBrowser::MODE_OPEN:
				if ((m_FileName.empty() == false) && (PathFileExistsA(filePath.c_str()) == TRUE))
				{
					SetFilePath(filePath.c_str());
					SetResult(FileBrowser::RESULT_OK);

					result = true;
				}
				else
				{
					ImGui::OpenPopup("Error");
				}
				break;
			case FileBrowser::MODE_SAVE:
				if (m_FileName.empty() == false)
				{
					if (m_Extensions.empty() == false)
					{
						auto it_begin = m_Extensions.begin();
						auto it_end = m_Extensions.end();

						bool hasExt = false;

						for (auto it = it_begin; (it != it_end) && (hasExt == false); ++it)
						{
							if ((filePath.size() > it->size()) && (filePath[filePath.size() - (it->size() + 1)] == '.'))
							{
								if (filePath.compare(filePath.size() - it->size(), it->size(), it->c_str()) == 0)
								{
									hasExt = true;
								}
							}
						}

						if (hasExt == false)
						{
							filePath += '.';
							filePath += m_Extensions[0];
						}
					}

					SetFilePath(filePath.c_str());
					SetResult(FileBrowser::RESULT_OK);

					result = true;
				}
				else
				{
					ImGui::OpenPopup("Error");
				}
				break;
			}
		}

		if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize) == true)
		{
			ImGui::Text(fileErrorMessage);
			ImGui::SameLine();

			if (ImGui::Button("Ok") == true)
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		return result;
	}

	void FileBrowser::SetDirectory(const char* pPath)
	{
		m_Directory = pPath;
		if ((m_Directory.back() != '\\') && (m_Directory.back() != '/'))
		{
			m_Directory += "\\";
		}

		m_ObjectInfos.clear();
		m_Objects.clear();

		StringA path = m_Directory + "*.*";

		WIN32_FIND_DATAA wfd{};
		HANDLE findHandle = FindFirstFileA(path.c_str(), &wfd);
		if (findHandle != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (((wfd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) == 0) && (strcmp(wfd.cFileName, ".") != 0))
				{
					bool directory = (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? true : false;
					bool find = false;

					if ((directory == false) && (m_Extensions.empty() == false))
					{
						auto it_being = m_Extensions.begin();
						auto it_end = m_Extensions.end();

						StringA fileName = wfd.cFileName;
						std::transform(fileName.begin(), fileName.end(), fileName.begin(), ::tolower);

						for (auto it = it_being; (it != it_end) && (find == false); ++it)
						{
							size_t fileNameLength = strlen(wfd.cFileName);
							size_t extLength = it->size();

							if ((fileNameLength > (extLength + 1)) &&
								(strcmp(&fileName[fileNameLength - extLength], it->c_str()) == 0) &&
								(fileName[fileNameLength - extLength - 1] == '.'))
							{
								find = true;
							}
						}
					}
					else
					{
						find = true;
					}

					if (find == true)
					{
						FileBrowser::ObjectInfo info;
						info.directory = directory;
						info.name = wfd.cFileName;
						m_ObjectInfos.push_back(info);
					}
				}

			} while (FindNextFileA(findHandle, &wfd) == TRUE);

			FindClose(findHandle);
		}

		if (m_ObjectInfos.empty() == false)
		{
			std::sort(m_ObjectInfos.begin(), m_ObjectInfos.end());

			m_Objects.reserve(m_ObjectInfos.size());

			auto it_begin = m_ObjectInfos.begin();
			auto it_end = m_ObjectInfos.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				m_Objects.push_back(it->name.c_str());
			}
		}

		m_ObjectIndex = 0;
	}

	void FileBrowser::UpDirectory()
	{
		size_t pos = m_Directory.rfind('\\', m_Directory.size() - 2);
		if (pos != StringA::npos)
		{
			StringA nextDirectory = m_Directory;
			nextDirectory.resize(pos + 1);

			SetDirectory(nextDirectory.c_str());
		}
	}

	void FileBrowser::DownDirectory(const char* pPath)
	{
		StringA nextDirectory;

		char lastChar = m_Directory[m_Directory.size() - 1];
		if ((lastChar != '\\') && (lastChar != '/'))
		{
			nextDirectory = m_Directory + "\\" + pPath;
		}
		else
		{
			nextDirectory = m_Directory + pPath;
		}

		SetDirectory(nextDirectory.c_str());
	}

}
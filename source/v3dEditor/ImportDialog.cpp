#include "ImportDialog.h"

namespace ve {

	ImportDialog::ImportDialog() : GuiPopup("New", true),
		m_FlipFaceEnable(true),
		m_InvertTexCoordUEnable(false),
		m_InvertTexCoordVEnable(true),
		m_InvertNormalEnable(false),
		m_TransformEnable(false),
		m_Scale(1.0f),
		m_OptimizeEnable(false),
		m_SommosingEnable(true),
		m_SmoosingAngle(30.0f),
		m_PathType(0)
	{
		m_Rotate[0] = 0.0f;
		m_Rotate[1] = 0.0f;
		m_Rotate[2] = 0.0f;

		m_Data = {};

		m_FileBrowser.SetCaption("Open File");
		m_FileBrowser.SetHash("Import_OpenFile");
		m_FileBrowser.SetMode(FileBrowser::MODE_OPEN);
		m_FileBrowser.AddExtension("fbx");
	}

	ImportDialog::~ImportDialog()
	{
	}

	const ImportDialog::Data& ImportDialog::GetData() const
	{
		return m_Data;
	}

	bool ImportDialog::OnRender()
	{
		bool result = false;

		SetResult(ImportDialog::RESULT_CANCEL);

		// ----------------------------------------------------------------------------------------------------
		// FilePath
		// ----------------------------------------------------------------------------------------------------

		ImGui::InputText("##ImportDialog_FilePath", &m_FilePath[0], static_cast<int32_t>(m_FilePath.size()), ImGuiInputTextFlags_ReadOnly);

		ImGui::SameLine();
		if (ImGui::Button("See###ImportDialog_File_See") == true)
		{
			m_FileBrowser.ShowModal();
		}

		ImGui::SameLine();
		ImGui::Text("Source");

		if (m_FileBrowser.Render() == FileBrowser::RESULT_OK)
		{
			m_FilePath = m_FileBrowser.GetFilePath();
		}

		ImGui::Spacing();

		// ----------------------------------------------------------------------------------------------------
		// Config - Load ModelSource
		// ----------------------------------------------------------------------------------------------------

		ImGui::Checkbox("FlipFace###ImportDialog_FlipFace", &m_FlipFaceEnable);
		ImGui::Checkbox("InvertTexCoordU###ImportDialog_InvertTexCoordUEnable", &m_InvertTexCoordUEnable);
		ImGui::Checkbox("InvertTexCoordV###ImportDialog_InvertTexCoordVEnable", &m_InvertTexCoordVEnable);
		ImGui::Checkbox("InvertNormal###ImportDialog_InvertNormalEnable", &m_InvertNormalEnable);

		ImGui::Checkbox("Transform###ImportDialog_TransformEnable", &m_TransformEnable);

		ImGui::Indent();
		if (m_TransformEnable == true)
		{
			if (ImGui::InputFloat3("Rotation##ImportDialog_Rotation", m_Rotate) == true)
			{
				VE_CLAMP(m_Rotate[0], -180.0f, +180.0f);
				VE_CLAMP(m_Rotate[1], -180.0f, +180.0f);
				VE_CLAMP(m_Rotate[2], -180.0f, +180.0f);
			}

			float tempScale = m_Scale;

			if (ImGui::InputFloat("Scale##ImportDialog_Scale", &tempScale) == true)
			{
				if (VE_FLOAT_EPSILON <= tempScale)
				{
					m_Scale = tempScale;
				}
			}
		}
		else
		{
			float tempRotate[3];
			float tempScale = m_Scale;

			tempRotate[0] = m_Rotate[0];
			tempRotate[1] = m_Rotate[1];
			tempRotate[2] = m_Rotate[2];

			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
			ImGui::InputFloat3("Rotation##ImportDialog_Rotation", tempRotate, -1, ImGuiInputTextFlags_ReadOnly);
			ImGui::InputFloat("Scale##ImportDialog_Scale", &tempScale, 0.0f, 0.0f, -1, ImGuiInputTextFlags_ReadOnly);
			ImGui::PopStyleVar();
		}
		ImGui::Unindent();

		// ----------------------------------------------------------------------------------------------------
		// Config - Load ModelRenderer
		// ----------------------------------------------------------------------------------------------------

		ImGui::Spacing();

		ImGui::Checkbox("Optimize###ImportDialog_OptimizeEnable", &m_OptimizeEnable);

		if (m_OptimizeEnable == true)
		{
			ImGui::Checkbox("Smoosing##ImportDialog_SmoosingEnable", &m_SommosingEnable);

			ImGui::Indent();

			if (m_SommosingEnable == true)
			{
				ImGui::SliderFloat("Angle###ImportDialog_SmoosingAngle", &m_SmoosingAngle, 0.0f, 90.0f);
			}
			else
			{
				float dummySmoosingAngle = m_SmoosingAngle;

				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
				ImGui::SliderFloat("Angle###ImportDialog_SmoosingAngle", &dummySmoosingAngle, 0.0f, 90.0f);
				ImGui::PopStyleVar();
			}

			ImGui::Unindent();
		}
		else
		{
			bool dummySmoosingEnable = m_SommosingEnable;
			float dummySmoosingAngle = m_SmoosingAngle;

			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);

			ImGui::Checkbox("Smoosing##ImportDialog_SmoosingEnable", &dummySmoosingEnable);

			ImGui::Indent();
			ImGui::SliderFloat("Angle###ImportDialog_SmoosingAngle", &dummySmoosingAngle, 0.0f, 90.0f);
			ImGui::Unindent();

			ImGui::PopStyleVar();
		}

		// ----------------------------------------------------------------------------------------------------
		// Config - PathType
		// ----------------------------------------------------------------------------------------------------

		ImGui::Spacing();

		ImGui::Combo("PathType###ImportDialog_PathType", &m_PathType, "Relative\0Strip\0\0");

		// ----------------------------------------------------------------------------------------------------

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (m_FilePath.empty() == false)
		{
			if (ImGui::Button("Ok###ImportDialog_Ok") == true)
			{
				SetResult(ImportDialog::RESULT_OK);
				result = true;
			}
		}
		else
		{
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
			ImGui::Button("Ok###ImportDialog_Ok");
			ImGui::PopStyleVar();
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel###ImportDialog_Cancel") == true)
		{
			result = true;
		}

		return result;
	}

	void ImportDialog::OnClose()
	{
		GuiPopup::OnClose();

		ToWideString(m_FilePath.c_str(), m_Data.sourceFilePath);

		m_Data.sourceConfig.flags = 0;
		if (m_FlipFaceEnable == true) { m_Data.sourceConfig.flags |= MODEL_SOURCE_FLIP_FACE; }
		if (m_InvertTexCoordUEnable == true) { m_Data.sourceConfig.flags |= MODEL_SOURCE_INVERT_TEXCOORD_U; }
		if (m_InvertTexCoordVEnable == true) { m_Data.sourceConfig.flags |= MODEL_SOURCE_INVERT_TEXCOORD_V; }
		if (m_InvertNormalEnable == true) { m_Data.sourceConfig.flags |= MODEL_SOURCE_INVERT_NORMAL; }
		if (m_TransformEnable == true) { m_Data.sourceConfig.flags |= MODEL_SOURCE_TRANSFORM; }

		if (m_TransformEnable == true)
		{
			glm::quat rx = glm::angleAxis(glm::radians(m_Rotate[0]), glm::vec3(1.0f, 0.0f, 0.0f));
			glm::quat ry = glm::angleAxis(glm::radians(m_Rotate[1]), glm::vec3(0.0f, 1.0f, 0.0f));
			glm::quat rz = glm::angleAxis(glm::radians(m_Rotate[2]), glm::vec3(0.0f, 0.0f, 1.0f));

			m_Data.sourceConfig.rotation = glm::toMat4(rz) * glm::toMat4(ry) * glm::toMat4(rx);
			m_Data.sourceConfig.scale = m_Scale;
		}
		else
		{
			m_Data.sourceConfig.rotation = glm::mat4(1.0f);
			m_Data.sourceConfig.scale = 1.0f;
		}

		m_Data.sourceConfig.pathType = (m_PathType == 0) ? MODEL_SOURCE_PATH_TYPE_RELATIVE : MODEL_SOURCE_PATH_TYPE_STRIP;

		m_Data.rednererConfig.optimizeEnable = m_OptimizeEnable;
		m_Data.rednererConfig.smoosingEnable = m_SommosingEnable;
		m_Data.rednererConfig.smoosingCos = glm::cos(glm::radians(m_SmoosingAngle));
	}

}
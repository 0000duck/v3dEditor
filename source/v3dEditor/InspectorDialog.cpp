#include "InspectorDialog.h"
#include "Scene.h"
#include "Node.h"
#include "Camera.h"
#include "Light.h"
#include "IMesh.h"
#include "IModel.h"
#include "Material.h"
#include "Texture.h"

namespace ve {

	InspectorDialog::InspectorDialog() : GuiFloat("Inspector", glm::ivec2(440, 680), ImGuiWindowFlags_ShowBorders),
		m_TextureFilter(0),
		m_TextureAddressMode(0),
		m_BlendMode(0),
		m_CullMode(0)
	{
		m_DiffuseTextureBrowser.SetCaption("Open DiffuseTexture File");
		m_DiffuseTextureBrowser.SetHash("Inspector_OpenDiffuseTextureFile");
		m_DiffuseTextureBrowser.SetMode(FileBrowser::MODE_OPEN);
		m_DiffuseTextureBrowser.AddExtension("dds");
		m_DiffuseTextureBrowser.AddExtension("ktx");

		m_SpecularTextureBrowser.SetCaption("Open SpecularTexture File");
		m_SpecularTextureBrowser.SetHash("Inspector_OpenSpecularTextureFile");
		m_SpecularTextureBrowser.SetMode(FileBrowser::MODE_OPEN);
		m_SpecularTextureBrowser.AddExtension("dds");
		m_SpecularTextureBrowser.AddExtension("ktx");

		m_BumpTextureBrowser.SetCaption("Open BumpTexture File");
		m_BumpTextureBrowser.SetHash("Inspector_OpenBumpTextureFile");
		m_BumpTextureBrowser.SetMode(FileBrowser::MODE_OPEN);
		m_BumpTextureBrowser.AddExtension("dds");
		m_BumpTextureBrowser.AddExtension("ktx");
	}

	InspectorDialog::~InspectorDialog()
	{
	}

	void InspectorDialog::SetDeviceContext(DeviceContextPtr deviceContext)
	{
		m_DeviceContext = deviceContext;
	}

	void InspectorDialog::SetScene(ScenePtr scene)
	{
		m_Scene = scene;
	}

	void InspectorDialog::SetNode(NodePtr node)
	{
		m_Node = node;

		if (m_Node != nullptr)
		{
			m_NodeAttribute = m_Node->GetAttribute();

			StringA caption;
			ToMultibyteString(m_Node->GetName(), caption);
			caption += " - Inspector";

			SetCaption(caption.c_str());
		}
		else
		{
			m_NodeAttribute = nullptr;
			SetCaption("Inspector");
		}

		m_StartNode = nullptr;

		m_PolygonCount = "";
		m_MeshCount = "";
		m_BoneCount = "";

		m_MaterialNameList.clear();
		m_MaterialList.clear();

		m_DiffuseTexture = "";
		m_SpecularTexture = "";
		m_BumpTexture = "";

		m_TextureFilter = 0;
		m_TextureAddressMode = 0;
		m_BlendMode = 0;
		m_CullMode = 0;

		if (m_NodeAttribute != nullptr)
		{
			NodeAttribute::TYPE type = m_NodeAttribute->GetType();

			if (type == NodeAttribute::TYPE_MODEL)
			{
				ModelPtr model = std::static_pointer_cast<IModel>(m_NodeAttribute);

				m_StartNode = m_Node;
				m_PolygonCount = std::to_string(model->GetPolygonCount());
				m_MeshCount = std::to_string(model->GetMeshCount());
			}
			else if (type == NodeAttribute::TYPE_MESH)
			{
				MeshPtr mesh = std::static_pointer_cast<IMesh>(m_NodeAttribute);

				m_StartNode = mesh->GetModel()->GetOwner();
				m_PolygonCount = std::to_string(mesh->GetPolygonCount());
				m_BoneCount = std::to_string(mesh->GetBoneCount());
			}
			else
			{
				m_StartNode = m_Node;
			}

			if ((type == NodeAttribute::TYPE_MODEL) || (type == NodeAttribute::TYPE_MESH))
			{
				MaterialContainerPtr materialContainer = std::dynamic_pointer_cast<IMaterialContainer>(m_NodeAttribute);

				m_MaterialNameList.clear();
				m_MaterialNameList.resize(materialContainer->GetMaterialCount());

				m_MaterialList.clear();
				m_MaterialList.resize(materialContainer->GetMaterialCount());

				m_DiffuseTexture = "";
				m_SpecularTexture = "";
				m_BumpTexture = "";

				for (uint32_t i = 0; i < materialContainer->GetMaterialCount(); i++)
				{
					ToMultibyteString(materialContainer->GetMaterial(i)->GetName(), m_MaterialNameList[i]);
					m_MaterialList[i] = m_MaterialNameList[i].c_str();
				}
			}

		}
	}

	void InspectorDialog::Dispose()
	{
		m_StartNode = nullptr;
		m_Node = nullptr;
		m_NodeAttribute = nullptr;
		m_Scene = nullptr;
		m_DeviceContext = nullptr;
	}

	bool InspectorDialog::OnRender()
	{
		if (m_Node == nullptr)
		{
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// Common
		// ----------------------------------------------------------------------------------------------------

		if (ImGui::CollapsingHeader("Transform###Inspector_Transform"))
		{
			if (ImGui::TreeNode("Local###Inspector_Transform_Local") == true)
			{
				float localS[3] = {};
				float localR[3] = {};
				float localT[3] = {};

				if (m_Node != nullptr)
				{
					const Transform& transform = m_Node->GetLocalTransform();
					glm::vec3 rot = glm::eulerAngles(transform.rotation);

					localS[0] = transform.scale.x;
					localS[1] = transform.scale.y;
					localS[2] = transform.scale.z;

					localR[0] = glm::degrees(rot.x);
					localR[1] = glm::degrees(rot.y);
					localR[2] = glm::degrees(rot.z);

					localT[0] = transform.translation.x;
					localT[1] = transform.translation.y;
					localT[2] = transform.translation.z;
				}

				if (ImGui::InputFloat3("Scale###Inspector_Transform_Local_Scale", localS) == true)
				{
					Transform transform = m_Node->GetLocalTransform();

					transform.scale.x = localS[0];
					transform.scale.y = localS[1];
					transform.scale.z = localS[2];

					m_Node->SetLocalTransform(transform);

					m_StartNode->Update();
				}

				if (ImGui::InputFloat3("Rotation###Inspector_Transform_Local_Rotation", localR) == true)
				{
					Transform transform = m_Node->GetLocalTransform();

					glm::vec3 rot;
					rot.x = glm::radians(localR[0]);
					rot.y = glm::radians(localR[1]);
					rot.z = glm::radians(localR[2]);

					transform.rotation = glm::quat(rot);

					m_Node->SetLocalTransform(transform);

					m_StartNode->Update();
				}

				if (ImGui::InputFloat3("Translation###Inspector_Transform_Local_Translation", localT) == true)
				{
					Transform transform = m_Node->GetLocalTransform();

					transform.translation.x = localT[0];
					transform.translation.y = localT[1];
					transform.translation.z = localT[2];

					m_Node->SetLocalTransform(transform);

					m_StartNode->Update();
				}

				ImGui::TreePop();
			}

			if (ImGui::TreeNode("World###Inspector_Transform_World") == true)
			{
				float worldS[3]{};
				float worldR[3]{};
				float worldT[3]{};

				const Transform& transform = m_Node->GetWorldTransform();
				glm::vec3 rot = glm::eulerAngles(transform.rotation);

				worldS[0] = transform.scale.x;
				worldS[1] = transform.scale.y;
				worldS[2] = transform.scale.z;

				worldR[0] = glm::degrees(rot.x);
				worldR[1] = glm::degrees(rot.y);
				worldR[2] = glm::degrees(rot.z);

				worldT[0] = transform.translation.x;
				worldT[1] = transform.translation.y;
				worldT[2] = transform.translation.z;

				ImGui::InputFloat3("Scale###Inspector_Transform_World_Scale", worldS, -1, ImGuiInputTextFlags_ReadOnly);
				ImGui::InputFloat3("Rotation###Inspector_Transform_World_Rotation", worldR, -1, ImGuiInputTextFlags_ReadOnly);
				ImGui::InputFloat3("Translation###Inspector_Transform_World_Translation", worldT, -1, ImGuiInputTextFlags_ReadOnly);

				ImGui::TreePop();
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// Individual
		// ----------------------------------------------------------------------------------------------------

		switch (m_NodeAttribute->GetType())
		{
		case NodeAttribute::TYPE_UNDEFINED:
			if (m_Scene->GetRootNode() == m_Node)
			{
				RenderScene();
			}
			break;
		case NodeAttribute::TYPE_CAMERA:
			RenderCamera();
			break;
		case NodeAttribute::TYPE_LIGHT:
			RenderLight();
			break;
		case NodeAttribute::TYPE_MODEL:
			RenderModel();
			break;
		case NodeAttribute::TYPE_MESH:
			RenderMesh();
			break;
		}

		// ----------------------------------------------------------------------------------------------------

		return false;
	}

	void InspectorDialog::RenderScene()
	{
		if (ImGui::CollapsingHeader("Grid###Inspector_Grid", ImGuiTreeNodeFlags_DefaultOpen))
		{
			Scene::Grid grid = m_Scene->GetGrid();
			bool gridUpdate = false;

			if (ImGui::Checkbox("Enable###Inspector_Grid_Enable", &grid.enable) == true)
			{
				gridUpdate = true;
			}

			if (ImGui::InputFloat("CellSize###Inspector_Grid_CellSize", &grid.cellSize) == true)
			{
				grid.cellSize = std::max(0.0f, grid.cellSize);
				gridUpdate = true;
			}

			int32_t halfCellCount = static_cast<int32_t>(grid.halfCellCount);
			if (ImGui::InputInt("HalfCellCount###Inspector_Grid_HalfCellCount", &halfCellCount) == true)
			{
				grid.halfCellCount = static_cast<uint32_t>(std::max(1, halfCellCount));
				gridUpdate = true;
			}

			float color[3] = { grid.color.r, grid.color.b, grid.color.b };
			if (ImGui::ColorEdit3("Color###Inspector_Grid_Color", color) == true)
			{
				grid.color.r = color[0];
				grid.color.g = color[1];
				grid.color.b = color[2];
				gridUpdate = true;
			}

			if (gridUpdate == true)
			{
				m_Scene->SetGrid(grid);
			}
		}

		if (ImGui::CollapsingHeader("AmbientOcclusion###Inspector_AmbientOcclusion", ImGuiTreeNodeFlags_DefaultOpen))
		{
			Scene::Ssao ssao = m_Scene->GetSsao();
			bool updateSsao = false;

			if (ImGui::Checkbox("Enable###Inspector_AmbientOcclusion_Enable", &ssao.enable) == true)
			{
				updateSsao = true;
			}

			if (ImGui::InputFloat("Radius###Inspector_AmbientOcclusion_Radius", &ssao.radius) == true)
			{
				updateSsao = true;
			}

			if (ImGui::InputFloat("Threshold###Inspector_AmbientOcclusion_Threshold", &ssao.threshold) == true)
			{
				updateSsao = true;
			}

			if (ImGui::InputFloat("Depth###Inspector_AmbientOcclusion_Depth", &ssao.depth) == true)
			{
				updateSsao = true;
			}

			if (ImGui::InputFloat("Intensity###Inspector_AmbientOcclusion_Intensity", &ssao.intensity) == true)
			{
				updateSsao = true;
			}

			if (ImGui::InputFloat("BlurRadius###Inspector_AmbientOcclusion_BlurRadius", &ssao.blurRadius) == true)
			{
				updateSsao = true;
			}

			if (ImGui::InputInt("BlurSamples###Inspector_AmbientOcclusion_BlurSamples", &ssao.blurSamples) == true)
			{
				updateSsao = true;
			}

			if (updateSsao == true)
			{
				m_Scene->SetSsao(ssao);
			}
		}

		if (ImGui::CollapsingHeader("Shadow###Inspector_Shadow", ImGuiTreeNodeFlags_DefaultOpen))
		{
			Scene::Shadow shadow = m_Scene->GetShadow();
			bool updateShadow = false;

			if (ImGui::Checkbox("Enable###Inspector_Shadow_Enable", &shadow.enable) == true)
			{
				updateShadow = true;
			}

			static char* resolutionList[] =
			{
				"1024",
				"2048",
				"4096",
			};
			
			int32_t resolutionIndex = shadow.resolution;

			if (ImGui::Combo("Resolution###Inspector_Shadow_Resolution", &resolutionIndex, resolutionList, _countof(resolutionList)) == true)
			{
				switch (resolutionIndex)
				{
				case 0:
					shadow.resolution = Scene::SHADOW_RESOLUTION_1024;
					break;
				case 1:
					shadow.resolution = Scene::SHADOW_RESOLUTION_2048;
					break;
				case 2:
					shadow.resolution = Scene::SHADOW_RESOLUTION_4096;
					break;

				default:
					shadow.resolution = Scene::SHADOW_RESOLUTION_1024;
				};

				updateShadow = true;
			}

			if (ImGui::InputFloat("Range###Inspector_Shadow_Range", &shadow.range) == true)
			{
				updateShadow = true;
			}

			if (ImGui::InputFloat("FadeMargin###Inspector_Shadow_FadeMargin", &shadow.fadeMargin) == true)
			{
				updateShadow = true;
			}

			if (ImGui::InputFloat("Dencity###Inspector_Shadow_Dencity", &shadow.dencity) == true)
			{
				updateShadow = true;
			}

			if (ImGui::InputFloat("CenterOffset###Inspector_Shadow_CenterOffset", &shadow.centerOffset) == true)
			{
				updateShadow = true;
			}

			if (ImGui::InputFloat("BoundsBias###Inspector_Shadow_BoundsBias", &shadow.boundsBias) == true)
			{
				updateShadow = true;
			}

			if (ImGui::SliderFloat("DepthBias###Inspector_Shadow_DepthBias", &shadow.depthBias, 0.0f, +0.01f, "%.4f") == true)
			{
				updateShadow = true;
			}

			if (updateShadow == true)
			{
				m_Scene->SetShadow(shadow);
				m_Scene->Update();
			}
		}

		if (ImGui::CollapsingHeader("Glare###Inspector_Glare", ImGuiTreeNodeFlags_DefaultOpen))
		{
			Scene::Glare glare = m_Scene->GetGlare();
			bool updateGlare = false;

			if (ImGui::Checkbox("Enable###Inspector_Glare_Enable", &glare.enable) == true)
			{
				updateGlare = true;
			}

			if (ImGui::InputFloat("BrightThreshold###Inspector_Glare_BrightThreshold", &glare.bpThreshold) == true)
			{
				updateGlare = true;
			}

			if (ImGui::InputFloat("BrightOffset###Inspector_Glare_BrightOffset", &glare.bpOffset) == true)
			{
				updateGlare = true;
			}

			if (ImGui::InputFloat("BlurRadius###Inspector_Glare_BlurRadius", &glare.blurRadius) == true)
			{
				updateGlare = true;
			}

			if (ImGui::InputInt("BlurSamples###Inspector_Glare_BlurSamples", &glare.blurSamples) == true)
			{
				updateGlare = true;
			}

			if (ImGui::InputFloat("Intensity1###Inspector_Glare_Intensity1", &glare.intensity[0]) == true)
			{
				updateGlare = true;
			}

			if (ImGui::InputFloat("Intensity2###Inspector_Glare_Intensity2", &glare.intensity[1]) == true)
			{
				updateGlare = true;
			}

			if (updateGlare == true)
			{
				m_Scene->SetGlare(glare);
			}
		}

		if (ImGui::CollapsingHeader("ImageEffect###Inspector_ImageEffect", ImGuiTreeNodeFlags_DefaultOpen))
		{
			Scene::ImageEffect imageEffect = m_Scene->GetImageEffect();

			ImGui::Checkbox("ToneMapping###Inspector_ImageEffect_ToneMapping", &imageEffect.toneMappingEnable);
			ImGui::Checkbox("Fxaa###Inspector_ImageEffect_Fxaa", &imageEffect.fxaaEnable);

			m_Scene->SetImageEffect(imageEffect);
		}

		if (ImGui::CollapsingHeader("Misc###Inspector_Misc", ImGuiTreeNodeFlags_DefaultOpen))
		{
			float tempColor[3];

			glm::vec4 bgColor = m_Scene->GetBackgroundColor();

			tempColor[0] = bgColor.r;
			tempColor[1] = bgColor.g;
			tempColor[2] = bgColor.b;

			if (ImGui::ColorEdit3("BackgroundColor###Inspector_Misc_BackgroundColor", tempColor) == true)
			{
				bgColor.r = tempColor[0];
				bgColor.g = tempColor[1];
				bgColor.b = tempColor[2];
				m_Scene->SetBackgroundColor(bgColor);
			}

			glm::vec4 selectedColor = m_Scene->GetSelectColor();

			tempColor[0] = selectedColor.r;
			tempColor[1] = selectedColor.g;
			tempColor[2] = selectedColor.b;

			if (ImGui::ColorEdit3("SelectedColor###Inspector_Misc_SelectedColor", tempColor) == true)
			{
				selectedColor.r = tempColor[0];
				selectedColor.g = tempColor[1];
				selectedColor.b = tempColor[2];
				m_Scene->SetSelectColor(selectedColor);
			}

			glm::vec4 meshBoundsColor = m_Scene->GetDebugColor(DEBUG_COLOR_TYPE_MESH_BOUNDS);

			tempColor[0] = meshBoundsColor.r;
			tempColor[1] = meshBoundsColor.g;
			tempColor[2] = meshBoundsColor.b;

			if (ImGui::ColorEdit3("MeshBoundsColor###Inspector_Misc_MeshBoundsColor", tempColor) == true)
			{
				meshBoundsColor.r = tempColor[0];
				meshBoundsColor.g = tempColor[1];
				meshBoundsColor.b = tempColor[2];
				m_Scene->SetDebugColor(DEBUG_COLOR_TYPE_MESH_BOUNDS, meshBoundsColor);
			}

			glm::vec4 lightShapeColor = m_Scene->GetDebugColor(DEBUG_COLOR_TYPE_LIGHT_SHAPE);

			tempColor[0] = lightShapeColor.r;
			tempColor[1] = lightShapeColor.g;
			tempColor[2] = lightShapeColor.b;

			if (ImGui::ColorEdit3("LightShapeColor###Inspector_Misc_LightShapeColor", tempColor) == true)
			{
				lightShapeColor.r = tempColor[0];
				lightShapeColor.g = tempColor[1];
				lightShapeColor.b = tempColor[2];
				m_Scene->SetDebugColor(DEBUG_COLOR_TYPE_LIGHT_SHAPE, lightShapeColor);
			}
		}
	}

	void InspectorDialog::RenderCamera()
	{
		if (ImGui::CollapsingHeader("Camera###Inspector_Camera", ImGuiTreeNodeFlags_DefaultOpen))
		{
			CameraPtr camera = std::static_pointer_cast<Camera>(m_NodeAttribute);

			float fovY = glm::degrees(camera->GetFovY());
			if (ImGui::SliderFloat("FovY###Inspactor_Camera_FovY", &fovY, 1.0f, 180.0f) == true)
			{
				camera->SetFovY(glm::radians(fovY));
			}

			float farClip = camera->GetFarClip();
			if (ImGui::InputFloat("FarClip###Inspactor_Camera_FarClip", &farClip) == true)
			{
				camera->SetFarClip(std::max(1.0f, farClip));
			}
		}
	}

	void InspectorDialog::RenderLight()
	{
		if (ImGui::CollapsingHeader("Light###Inspector_Light", ImGuiTreeNodeFlags_DefaultOpen))
		{
			LightPtr light = std::static_pointer_cast<Light>(m_NodeAttribute);

			glm::vec4 color = light->GetColor();
			float tempColor[3] = { color.r, color.g, color.b };
			if (ImGui::ColorEdit3("Color###Inspector_Light_Color", tempColor) == true)
			{
				color.r = tempColor[0];
				color.g = tempColor[1];
				color.b = tempColor[2];
				light->SetColor(color);
			}

			Transform localTransform = m_Node->GetLocalTransform();
			glm::vec3 rot = glm::eulerAngles(localTransform.rotation);
			float angles[3] = { glm::degrees(rot.x), glm::degrees(rot.y), glm::degrees(rot.z) };

			if (ImGui::SliderFloat3("Rotation###Inspector_Light_Rotation", angles, -180.0f, +180.0f) == true)
			{
				rot.x = glm::radians(angles[0]);
				rot.y = glm::radians(angles[1]);
				rot.z = glm::radians(angles[2]);

				localTransform.rotation = glm::quat(rot);

				m_Node->SetLocalTransform(localTransform);
				m_Node->Update();
			}
		}
	}

	void InspectorDialog::RenderModel()
	{
		ModelPtr model = std::static_pointer_cast<IModel>(m_NodeAttribute);

		/*********/
		/* Model */
		/*********/

		if (ImGui::CollapsingHeader("Model###Inspector_Model", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::InputText("Polygons###Inspector_Model_Polygons", &m_PolygonCount[0], m_PolygonCount.size(), ImGuiInputTextFlags_ReadOnly);
			ImGui::InputText("Meshes###Inspector_Model_Meshes", &m_MeshCount[0], m_MeshCount.size(), ImGuiInputTextFlags_ReadOnly);
		}

		/************/
		/* Material */
		/************/

		RenderMaterialContainer(model);
	}

	void InspectorDialog::RenderMesh()
	{
		MeshPtr mesh = std::static_pointer_cast<IMesh>(m_NodeAttribute);

		/********/
		/* Mesh */
		/********/

		if (ImGui::CollapsingHeader("Mesh###Inspector_Mesh", ImGuiTreeNodeFlags_DefaultOpen))
		{
			bool visible = mesh->GetVisible();
			bool castShadow = mesh->GetCastShadow();

			ImGui::InputText("Polygons###Inspector_Mesh_Polygons", &m_PolygonCount[0], m_PolygonCount.size(), ImGuiInputTextFlags_ReadOnly);
			ImGui::InputText("Bones###Inspector_Mesh_Bones", &m_BoneCount[0], m_BoneCount.size(), ImGuiInputTextFlags_ReadOnly);
			ImGui::Spacing();

			if (ImGui::Checkbox("Visible###Inspector_Mesh_Visible", &visible) == true)
			{
				mesh->SetVisible(visible);
			}

			if (ImGui::Checkbox("CastShadow###Inspector_Mesh_CastShadow", &castShadow) == true)
			{
				mesh->SetCastShadow(castShadow);
			}

			ImGui::Spacing();
		}

		/************/
		/* Material */
		/************/

		RenderMaterialContainer(mesh);
	}

	void InspectorDialog::RenderMaterialContainer(MaterialContainerPtr materialContainer)
	{
		if (ImGui::CollapsingHeader("Material###Inspector_Material", ImGuiTreeNodeFlags_DefaultOpen))
		{

			static int materialIndex = 0;

			int32_t materialCount = static_cast<int32_t>(m_MaterialList.size());
			if (materialCount <= materialIndex)
			{
				materialIndex = (materialCount > 0) ? (materialCount - 1) : 0;
			}

			MaterialPtr material = materialContainer->GetMaterial(materialIndex);

			const glm::vec4& srcDiffuseColor = material->GetDiffuseColor();
			float diffuseColor[4] =
			{
				srcDiffuseColor.r,
				srcDiffuseColor.g,
				srcDiffuseColor.b,
				srcDiffuseColor.a,
			};

			float diffuseFactor = material->GetDiffuseFactor();

			const glm::vec4& srcSpecularColor = material->GetSpecularColor();
			float specularColor[3] =
			{
				srcSpecularColor.r,
				srcSpecularColor.g,
				srcSpecularColor.b,
			};

			float emissiveFactor = material->GetEmissiveFactor();

			float specularFactor = material->GetSpecularFactor();
			float shiness = material->GetShininess();

			TexturePtr diffuseTexture = material->GetDiffuseTexture();
			if (diffuseTexture != nullptr)
			{
				ToMultibyteString(diffuseTexture->GetFilePath(), m_DiffuseTexture);
			}

			TexturePtr specularTexture = material->GetSpecularTexture();
			if (specularTexture != nullptr)
			{
				ToMultibyteString(specularTexture->GetFilePath(), m_SpecularTexture);
			}

			TexturePtr bumpTexture = material->GetBumpTexture();
			if (bumpTexture != nullptr)
			{
				ToMultibyteString(bumpTexture->GetFilePath(), m_BumpTexture);
			}

			int32_t texFilter = material->GetTextureFilter();
			int32_t texAddressMode = material->GetTextureAddressMode();

			int32_t blendMode = material->GetBlendMode();
			int32_t cullMode = material->GetCullMode();

			ImGui::ListBox("##Inspector_Material_List", &materialIndex, m_MaterialList.data(), materialCount, 4);

			/****************/
			/* DiffuseColor */
			/****************/

			if (ImGui::ColorEdit4("DiffuseColor###Inspector_Material_DiffuseColor", diffuseColor) == true)
			{
				if (material != nullptr)
				{
					material->SetDiffuseColor(glm::vec4(diffuseColor[0], diffuseColor[1], diffuseColor[2], diffuseColor[3]));
					material->Update();
				}
			}

			if (ImGui::SliderFloat("DiffuseFactor###Inspector_Material_DiffuseFactor", &diffuseFactor, 0.0f, 1.0f) == true)
			{
				if (material != nullptr)
				{
					material->SetDiffuseFactor(diffuseFactor);
					material->Update();
				}
			}

			/******************/
			/* EmissiveFactor */
			/******************/

			if (ImGui::SliderFloat("EmissiveFactor###Inspector_Material_EmissiveFactor", &emissiveFactor, 0.0f, 1.0f) == true)
			{
				if (material != nullptr)
				{
					material->SetEmissiveFactor(emissiveFactor);
					material->Update();
				}
			}

			/******************/
			/* SpecularFactor */
			/******************/

			if (ImGui::ColorEdit3("SpecularColor###Inspector_Material_SpecularColor", specularColor) == true)
			{
				if (material != nullptr)
				{
					material->SetSpecularColor(glm::vec4(specularColor[0], specularColor[1], specularColor[2], 1.0f));
					material->Update();
				}
			}

			if (ImGui::SliderFloat("SpecularFactor###Inspector_Material_SpecularFactor", &specularFactor, 0.0f, 1.0f) == true)
			{
				if (material != nullptr)
				{
					material->SetSpecularFactor(specularFactor);
					material->Update();
				}
			}

			/***********/
			/* Shiness */
			/***********/

			if (ImGui::SliderFloat("Shiness###Inspector_Material_Shiness", &shiness, 1.0f, 100.0f) == true)
			{
				if (material != nullptr)
				{
					material->SetShininess(shiness);
					material->Update();
				}
			}

			/******************/
			/* DiffuseTexture */
			/******************/

			ImGui::InputText("DiffuseTexture##Inspector_Material_DiffuseTexture", &m_DiffuseTexture[0], static_cast<int32_t>(m_DiffuseTexture.size()), ImGuiInputTextFlags_ReadOnly);

			if (ImGui::Button("Open###Inspector_Material_DiffuseTexture_Open") == true)
			{
				if (material != nullptr)
				{
					m_DiffuseTextureBrowser.SetFilePath(m_DiffuseTexture.c_str());
					m_DiffuseTextureBrowser.ShowModal();
				}
			}

			if (m_DiffuseTextureBrowser.Render() == FileBrowser::RESULT_OK)
			{
				StringW filePath;
				ToWideString(m_DiffuseTextureBrowser.GetFilePath(), filePath);

				TexturePtr texture = Texture::Create(m_DeviceContext, filePath.c_str());
				if (texture != nullptr)
				{
					material->SetDiffuseTexture(texture);
					material->Update();

					m_DiffuseTexture = m_DiffuseTextureBrowser.GetFilePath();
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Clear###Inspector_Material_DiffuseTexture_Clear") == true)
			{
				if (material != nullptr)
				{
					material->SetDiffuseTexture(nullptr);
					material->Update();

					m_DiffuseTexture = "";
				}
			}

			/*******************/
			/* SpecularTexture */
			/*******************/

			ImGui::InputText("SpecularTexture###Inspector_Material_SpecularTexture", &m_SpecularTexture[0], static_cast<int32_t>(m_SpecularTexture.size()), ImGuiInputTextFlags_ReadOnly);

			if (ImGui::Button("Open###Inspector_Material_SpecularTexture_Open") == true)
			{
				if (material != nullptr)
				{
					m_SpecularTextureBrowser.SetFilePath(m_SpecularTexture.c_str());
					m_SpecularTextureBrowser.ShowModal();
				}
			}

			if (m_SpecularTextureBrowser.Render() == FileBrowser::RESULT_OK)
			{
				StringW filePath;
				ToWideString(m_SpecularTextureBrowser.GetFilePath(), filePath);

				TexturePtr texture = Texture::Create(m_DeviceContext, filePath.c_str());
				if (texture != nullptr)
				{
					material->SetSpecularTexture(texture);
					material->Update();

					m_SpecularTexture = m_SpecularTextureBrowser.GetFilePath();
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Clear###Inspector_Material_SpecularTexture_Clear") == true)
			{
				if (material != nullptr)
				{
					material->SetSpecularTexture(nullptr);
					material->Update();

					m_SpecularTexture = "";
				}
			}

			/***************/
			/* BumpTexture */
			/***************/

			ImGui::InputText("BumpTexture###Inspector_Material_BumpTexture", &m_BumpTexture[0], static_cast<int32_t>(m_BumpTexture.size()), ImGuiInputTextFlags_ReadOnly);

			if (ImGui::Button("Open###Inspector_Material_BumpTexture_Open") == true)
			{
				if (material != nullptr)
				{
					m_BumpTextureBrowser.SetFilePath(m_BumpTexture.c_str());
					m_BumpTextureBrowser.ShowModal();
				}
			}

			if (m_BumpTextureBrowser.Render() == FileBrowser::RESULT_OK)
			{
				StringW filePath;
				ToWideString(m_BumpTextureBrowser.GetFilePath(), filePath);

				TexturePtr texture = Texture::Create(m_DeviceContext, filePath.c_str());
				if (texture != nullptr)
				{
					material->SetBumpTexture(texture);
					material->Update();

					m_BumpTexture = m_BumpTextureBrowser.GetFilePath();
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Clear###Inspector_Material_BumpTexture_Clear") == true)
			{
				if (material != nullptr)
				{
					material->SetBumpTexture(nullptr);
					material->Update();

					m_BumpTexture = "";
				}
			}

			/*****************/
			/* TextureFilter */
			/*****************/

			if (ImGui::Combo("TextureFilter###Inspector_Material_TextureFilter", &texFilter, InspectorDialog::TextureFilters, _countof(InspectorDialog::TextureFilters)) == true)
			{
				if (material != nullptr)
				{
					material->SetTextureFilter(static_cast<V3D_FILTER>(texFilter));
					material->Update();
				}
			}

			/**********************/
			/* TextureAddressMode */
			/**********************/

			if (ImGui::Combo("TextureAddressMode###Inspector_Material_TextureAddressMode", &texAddressMode, InspectorDialog::TextureAddressModes, _countof(InspectorDialog::TextureAddressModes)) == true)
			{
				if (material != nullptr)
				{
					material->SetTextureAddressMode(static_cast<V3D_ADDRESS_MODE>(texAddressMode));
					material->Update();
				}
			}

			/*************/
			/* BlendMode */
			/*************/

			if (ImGui::Combo("BlendMode###Inspector_Material_BlendMode", &blendMode, InspectorDialog::BlendModes, _countof(InspectorDialog::BlendModes)) == true)
			{
				if (material != nullptr)
				{
					material->SetBlendMode(static_cast<BLEND_MODE>(blendMode));
					material->Update();
				}
			}

			/************/
			/* CullMode */
			/************/

			if (ImGui::Combo("CullMode###Inspector_Material_CullMode", &cullMode, InspectorDialog::CullModes, _countof(InspectorDialog::CullModes)) == true)
			{
				if (material != nullptr)
				{
					material->SetCullMode(static_cast<V3D_CULL_MODE>(cullMode));
					material->Update();
				}
			}
		}
	}

}
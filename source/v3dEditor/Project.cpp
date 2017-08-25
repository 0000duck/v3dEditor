#include "Project.h"
#include "BackgroundQueue.h"
#include "FbxModelSource.h"
#include "Scene.h"
#include "SkeletalModel.h"
#include "Node.h"
#include "Camera.h"
#include "Light.h"
#include "IModel.h"

namespace ve {

	ProjectPtr Project::Create()
	{
		return std::move(std::make_shared<Project>());
	}

	Project::Project()
	{
	}

	Project::~Project()
	{
	}

	void Project::New(const wchar_t* pFilePath)
	{
		RemoveFileExtensionW(pFilePath, m_FilePath);
		RemoveFileSpecW(pFilePath, m_DirPath);
	}

	bool Project::Open(LoggerPtr logger, DeviceContextPtr deviceContext, ScenePtr scene, const wchar_t* pFilePath)
	{
		RemoveFileExtensionW(pFilePath, m_FilePath);
		RemoveFileSpecW(pFilePath, m_DirPath);

		xml::Element* pDocument = xml::Load(pFilePath);
		if (pDocument == nullptr)
		{
			return false;
		}

		if (pDocument->childs.size() != 1)
		{
			pDocument->Destroy();
			return false;
		}

		xml::Element* pRoot = pDocument->childs[0];
		if (pRoot->name != L"v3dEditor")
		{
			pDocument->Destroy();
			return false;
		}

		if (LoadRoot(pRoot, logger, deviceContext, scene) == false)
		{
			pDocument->Destroy();
			return false;
		}

		pDocument->Destroy();

		scene->GetRootNode()->Update();

		return true;
	}

	bool Project::Save(LoggerPtr logger, ScenePtr scene)
	{
		xml::Element* pDocument = xml::Element::Create();
		if (pDocument == nullptr)
		{
			return false;
		}

		if (SaveRoot(pDocument, logger, scene) == false)
		{
			pDocument->Destroy();
			return false;
		}

		StringW filePath = m_FilePath + L".xml";
		if (xml::Save(filePath.c_str(), pDocument) == false)
		{
			pDocument->Destroy();
			return false;
		}

		pDocument->Destroy();

		return true;
	}

	const wchar_t* Project::GetFilePathWithoutExtension() const
	{
		return m_FilePath.c_str();
	}

	/*********************/
	/* private - Prokect */
	/*********************/

	bool Project::LoadRoot(xml::Element* pElement, LoggerPtr logger, DeviceContextPtr deviceContext, ScenePtr scene)
	{
		const xml::Attribute* pAttribute = xml::FindAttribute(pElement, L"version");
		if (pAttribute == nullptr)
		{
			return false;
		}

		if (pAttribute->value != Project::XmlV3DEditorVersion)
		{
			return false;
		}

		auto it_child_begin = pElement->childs.begin();
		auto it_child_end = pElement->childs.end();

		for (auto it_child = it_child_begin; it_child != it_child_end; ++it_child)
		{
			xml::Element* pChild = *it_child;

			if (pChild->name == L"scene")
			{
				if (LoadScene(*it_child, logger, deviceContext, scene) == false)
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}

		return true;
	}

	bool Project::LoadScene(xml::Element* pElement, LoggerPtr logger, DeviceContextPtr deviceContext, ScenePtr scene)
	{
		const xml::Attribute* pAttribute = xml::FindAttribute(pElement, L"version");
		if (pAttribute == nullptr)
		{
			return false;
		}

		if (pAttribute->value != Project::XmlSceneVersion)
		{
			return false;
		}

		auto it_child_begin = pElement->childs.begin();
		auto it_child_end = pElement->childs.end();

		bool rootNodeLoaded = false;

		for (auto it_child = it_child_begin; it_child != it_child_end; ++it_child)
		{
			xml::Element* pChild = *it_child;

			if (pChild->name == L"grid")
			{
				if (LoadGrid(pChild, scene) == false)
				{
					return false;
				}
			}
			else if (pChild->name == L"ssao")
			{
				if (LoadSsao(pChild, scene) == false)
				{
					return false;
				}
			}
			else if (pChild->name == L"shadow")
			{
				if (LoadShadow(pChild, scene) == false)
				{
					return false;
				}
			}
			else if (pChild->name == L"glare")
			{
				if (LoadGlare(pChild, scene) == false)
				{
					return false;
				}
			}
			else if (pChild->name == L"imageEffect")
			{
				if (LoadImageEffect(pChild, scene) == false)
				{
					return false;
				}
			}
			else if (pChild->name == L"misc")
			{
				if (LoadMisc(pChild, scene) == false)
				{
					return false;
				}
			}
			else if (pChild->name == L"node")
			{
				if (rootNodeLoaded == true)
				{
					return false;
				}

				if (LoadNode(*it_child, logger, deviceContext, scene, scene->GetRootNode()) == false)
				{
					return false;
				}

				rootNodeLoaded = true;
			}
			else
			{
				return false;
			}
		}

		return true;
	}

	bool Project::LoadGrid(xml::Element* pElement, ScenePtr scene)
	{
		auto it_begin = pElement->attributes.begin();
		auto it_end = pElement->attributes.end();

		Scene::Grid grid = scene->GetGrid();

		for (auto it = it_begin; it != it_end; ++it)
		{
			if (wcscmp(it->name.c_str(), L"enable") == 0)
			{
				grid.enable = std::stoi(it->value.c_str());
			}
			else if (wcscmp(it->name.c_str(), L"cellSize") == 0)
			{
				grid.cellSize = std::stof(it->value.c_str());
			}
			else if (wcscmp(it->name.c_str(), L"halfCellCount") == 0)
			{
				grid.halfCellCount = std::stoul(it->value.c_str());
			}
		}

		scene->SetGrid(grid);

		return true;
	}

	bool Project::LoadSsao(xml::Element* pElement, ScenePtr scene)
	{
		auto it_begin = pElement->attributes.begin();
		auto it_end = pElement->attributes.end();

		Scene::Ssao ssao = scene->GetSsao();

		for (auto it = it_begin; it != it_end; ++it)
		{
			if (wcscmp(it->name.c_str(), L"enable") == 0)
			{
				ssao.enable = std::stoi(it->value.c_str());
			}
			else if (wcscmp(it->name.c_str(), L"radius") == 0)
			{
				ssao.radius = std::stof(it->value.c_str());
			}
			else if (wcscmp(it->name.c_str(), L"threshold") == 0)
			{
				ssao.threshold = std::stof(it->value.c_str());
			}
			else if (wcscmp(it->name.c_str(), L"depth") == 0)
			{
				ssao.depth = std::stof(it->value.c_str());
			}
			else if (wcscmp(it->name.c_str(), L"intensity") == 0)
			{
				ssao.intensity = std::stof(it->value.c_str());
			}
			else if (wcscmp(it->name.c_str(), L"blurRadius") == 0)
			{
				ssao.blurRadius = std::stof(it->value.c_str());
			}
			else if (wcscmp(it->name.c_str(), L"blurSamples") == 0)
			{
				ssao.blurSamples = std::stoi(it->value.c_str());
			}
		}

		scene->SetSsao(ssao);

		return true;
	}

	bool Project::LoadShadow(xml::Element* pElement, ScenePtr scene)
	{
		auto it_begin = pElement->attributes.begin();
		auto it_end = pElement->attributes.end();

		Scene::Shadow shadow = scene->GetShadow();

		for (auto it = it_begin; it != it_end; ++it)
		{
			if (wcscmp(it->name.c_str(), L"enable") == 0)
			{
				shadow.enable = std::stoi(it->value.c_str());
			}
			else if (wcscmp(it->name.c_str(), L"resolution") == 0)
			{
				shadow.resolution = static_cast<Scene::SHADOW_RESOLUTION>(std::stoi(it->value.c_str()));
			}
			else if (wcscmp(it->name.c_str(), L"range") == 0)
			{
				shadow.range = std::stof(it->value.c_str());
			}
			else if (wcscmp(it->name.c_str(), L"fadeMargin") == 0)
			{
				shadow.fadeMargin = std::stof(it->value.c_str());
			}
			else if (wcscmp(it->name.c_str(), L"dencity") == 0)
			{
				shadow.dencity = std::stof(it->value.c_str());
			}
			else if (wcscmp(it->name.c_str(), L"centerOffset") == 0)
			{
				shadow.centerOffset = std::stof(it->value.c_str());
			}
			else if (wcscmp(it->name.c_str(), L"boundsBias") == 0)
			{
				shadow.boundsBias = std::stof(it->value.c_str());
			}
			else if (wcscmp(it->name.c_str(), L"depthBias") == 0)
			{
				shadow.depthBias = std::stof(it->value.c_str());
			}
		}

		scene->SetShadow(shadow);

		return true;
	}

	bool Project::LoadGlare(xml::Element* pElement, ScenePtr scene)
	{
		auto it_begin = pElement->attributes.begin();
		auto it_end = pElement->attributes.end();

		Scene::Glare glare = scene->GetGlare();

		for (auto it = it_begin; it != it_end; ++it)
		{
			if (wcscmp(it->name.c_str(), L"enable") == 0)
			{
				glare.enable = std::stoi(it->value.c_str());
			}
			else if (wcscmp(it->name.c_str(), L"bpThreshold") == 0)
			{
				glare.bpThreshold = std::stof(it->value.c_str());
			}
			else if (wcscmp(it->name.c_str(), L"bpOffset") == 0)
			{
				glare.bpOffset = std::stof(it->value.c_str());
			}
			else if (wcscmp(it->name.c_str(), L"blurRadius") == 0)
			{
				glare.blurRadius = std::stof(it->value.c_str());
			}
			else if (wcscmp(it->name.c_str(), L"blurSamples") == 0)
			{
				glare.blurSamples = std::stoul(it->value.c_str());
			}
			else if (wcscmp(it->name.c_str(), L"intensity0") == 0)
			{
				glare.intensity[0] = std::stof(it->value.c_str());
			}
			else if (wcscmp(it->name.c_str(), L"intensity1") == 0)
			{
				glare.intensity[1] = std::stof(it->value.c_str());
			}
		}

		scene->SetGlare(glare);

		return true;
	}

	bool Project::LoadImageEffect(xml::Element* pElement, ScenePtr scene)
	{
		auto it_begin = pElement->attributes.begin();
		auto it_end = pElement->attributes.end();

		Scene::ImageEffect imageEffect = scene->GetImageEffect();

		for (auto it = it_begin; it != it_end; ++it)
		{
			if (wcscmp(it->name.c_str(), L"toneMappingEnable") == 0)
			{
				imageEffect.toneMappingEnable = std::stoi(it->value.c_str());
			}
			else if (wcscmp(it->name.c_str(), L"fxaaEnable") == 0)
			{
				imageEffect.fxaaEnable = std::stoi(it->value.c_str());
			}
		}

		scene->SetImageEffect(imageEffect);

		return true;
	}

	bool Project::LoadMisc(xml::Element* pElement, ScenePtr scene)
	{
		auto it_child_begin = pElement->childs.begin();
		auto it_child_end = pElement->childs.end();

		for (auto it_child = it_child_begin; it_child != it_child_end; ++it_child)
		{
			xml::Element* pChild = *it_child;

			if (pChild->name == L"background")
			{
				const xml::Attribute* pAttribute = xml::FindAttribute(pChild, L"color");

				glm::vec4 color;
				if (Project::ParseColor(pAttribute->value, color) == true)
				{
					scene->SetBackgroundColor(color);
				}
			}
			else if (pChild->name == L"select")
			{
				const xml::Attribute* pAttribute = xml::FindAttribute(pChild, L"color");

				glm::vec4 color;
				if (Project::ParseColor(pAttribute->value, color) == true)
				{
					scene->SetSelectColor(color);
				}
			}
			else if (pChild->name == L"lightShape")
			{
				const xml::Attribute* pEnableAttribute = xml::FindAttribute(pChild, L"enable");
				if (pEnableAttribute != nullptr)
				{
					uint32_t flags = scene->GetDebugDrawFlags();
					if (std::stoi(pEnableAttribute->value) > 0)
					{
						VE_SET_BIT(flags, DEBUG_DRAW_LIGHT_SHAPE);
					}
					else
					{
						VE_RESET_BIT(flags, DEBUG_DRAW_LIGHT_SHAPE);
					}

					scene->SetDebugDrawFlags(flags);
				}

				const xml::Attribute* pColorAttribute = xml::FindAttribute(pChild, L"color");
				glm::vec4 color;
				if (Project::ParseColor(pColorAttribute->value, color) == true)
				{
					scene->SetDebugColor(DEBUG_COLOR_TYPE_LIGHT_SHAPE, color);
				}
			}
			else if (pChild->name == L"meshBounds")
			{
				const xml::Attribute* pEnableAttribute = xml::FindAttribute(pChild, L"enable");
				if (pEnableAttribute != nullptr)
				{
					uint32_t flags = scene->GetDebugDrawFlags();
					if (std::stoi(pEnableAttribute->value) > 0)
					{
						VE_SET_BIT(flags, DEBUG_DRAW_MESH_BOUNDS);
					}
					else
					{
						VE_RESET_BIT(flags, DEBUG_DRAW_MESH_BOUNDS);
					}

					scene->SetDebugDrawFlags(flags);
				}

				const xml::Attribute* pColorAttribute = xml::FindAttribute(pChild, L"color");
				glm::vec4 color;
				if (Project::ParseColor(pColorAttribute->value, color) == true)
				{
					scene->SetDebugColor(DEBUG_COLOR_TYPE_MESH_BOUNDS, color);
				}
			}
		}

		return true;
	}

	bool Project::ParseColor(const StringW& str, glm::vec4& color)
	{
		collection::Vector<StringW> values;

		values.clear();
		ParseStringW(str, L", ", values);

		if (values.size() == 3)
		{
			color.r = std::stof(values[0]);
			color.g = std::stof(values[1]);
			color.b = std::stof(values[2]);
			color.a = 1.0f;
		}
		else if (values.size() == 4)
		{
			color.r = std::stof(values[0]);
			color.g = std::stof(values[1]);
			color.b = std::stof(values[2]);
			color.a = std::stof(values[3]);
		}
		else
		{
			return false;
		}

		return true;
	}

	bool Project::LoadNode(xml::Element* pElement, LoggerPtr logger, DeviceContextPtr deviceContext, ScenePtr scene, NodePtr node)
	{
		const xml::Attribute* pNameAttribute = xml::FindAttribute(pElement, L"name");
		if (pNameAttribute == nullptr)
		{
			return false;
		}

		const xml::Attribute* pAttribute = xml::FindAttribute(pElement, L"attribute");
		if (pAttribute == nullptr)
		{
			return false;
		}

		NodePtr newNode;

		if (pAttribute->value == L"undefined")
		{
			if (scene->GetRootNode() != node)
			{
				return false;
			}

			node->SetName(pNameAttribute->value.c_str());
			newNode = node;
		}
		else if (pAttribute->value == L"empty")
		{
		}
		else if (pAttribute->value == L"camera")
		{
			newNode = LoadCamera(pElement, scene);
		}
		else if (pAttribute->value == L"light")
		{
			newNode = LoadLight(pElement, scene);
		}
		else if (pAttribute->value == L"model")
		{
			newNode = LoadModel(pElement, logger, deviceContext, scene, node);
		}
		else
		{
			return false;
		}

		if (newNode == nullptr)
		{
			return false;
		}

		collection::Vector<StringW> values;
		Transform transform;

		const xml::Attribute* pScaleAttribute = xml::FindAttribute(pElement, L"ls");
		if (pScaleAttribute != nullptr)
		{
			values.clear();
			ParseStringW(pScaleAttribute->value, L", ", values);
			if (values.size() != 3)
			{
				return false;
			}

			transform.scale.x = std::stof(values[0]);
			transform.scale.y = std::stof(values[1]);
			transform.scale.z = std::stof(values[2]);
		}
		else
		{
			transform.scale.x = 1.0f;
			transform.scale.y = 1.0f;
			transform.scale.z = 1.0f;
		}

		const xml::Attribute* pRotationAttribute = xml::FindAttribute(pElement, L"lr");
		if (pRotationAttribute != nullptr)
		{
			values.clear();
			ParseStringW(pRotationAttribute->value, L", ", values);
			if (values.size() != 4)
			{
				return false;
			}

			transform.rotation.x = std::stof(values[0]);
			transform.rotation.y = std::stof(values[1]);
			transform.rotation.z = std::stof(values[2]);
			transform.rotation.w = std::stof(values[3]);
		}
		else
		{
			transform.rotation.x = 0.0f;
			transform.rotation.y = 0.0f;
			transform.rotation.z = 0.0f;
			transform.rotation.w = 1.0f;
		}

		const xml::Attribute* pTranslationAttribute = xml::FindAttribute(pElement, L"lt");
		if (pTranslationAttribute != nullptr)
		{
			values.clear();
			ParseStringW(pTranslationAttribute->value, L", ", values);
			if (values.size() != 3)
			{
				return false;
			}

			transform.translation.x = std::stof(values[0]);
			transform.translation.y = std::stof(values[1]);
			transform.translation.z = std::stof(values[2]);
		}
		else
		{
			transform.translation.x = 0.0f;
			transform.translation.y = 0.0f;
			transform.translation.z = 0.0f;
		}

		newNode->SetLocalTransform(transform);

		auto it_child_begin = pElement->childs.begin();
		auto it_child_end = pElement->childs.end();

		for (auto it_child = it_child_begin; it_child != it_child_end; ++it_child)
		{
			xml::Element* pChild = *it_child;

			if (pChild->name == L"node")
			{
				if (LoadNode(*it_child, logger, deviceContext, scene, newNode) == false)
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}

		return true;
	}

	NodePtr Project::LoadAttribute(xml::Element* pElement, ScenePtr scene, NodePtr parent)
	{
		return nullptr;
	}

	NodePtr Project::LoadCamera(xml::Element* pElement, ScenePtr scene)
	{
		NodePtr node = scene->GetRootNode()->FindChild(L"Camera");

		auto it_begin = pElement->attributes.begin();
		auto it_end = pElement->attributes.end();

		CameraPtr camera = std::static_pointer_cast<Camera>(node->GetAttribute());
		collection::Vector<StringW> values;

		const xml::Attribute* pRotationAttribute = xml::FindAttribute(pElement, L"rotation");
		if (pRotationAttribute != nullptr)
		{
			values.clear();
			ParseStringW(pRotationAttribute->value, L", ", values);
			if (values.size() != 4)
			{
				return nullptr;
			}

			camera->SetRotation(glm::quat(std::stof(values[3]), std::stof(values[0]), std::stof(values[1]), std::stof(values[2])));
		}

		const xml::Attribute* pAtAttribute = xml::FindAttribute(pElement, L"at");
		if (pAtAttribute != nullptr)
		{
			values.clear();
			ParseStringW(pAtAttribute->value, L", ", values);
			if (values.size() != 3)
			{
				return nullptr;
			}

			camera->SetAt(glm::vec3(std::stof(values[0]), std::stof(values[1]), std::stof(values[2])));
		}

		const xml::Attribute* pDistanceAttribute = xml::FindAttribute(pElement, L"distance");
		if (pDistanceAttribute != nullptr)
		{
			camera->SetDistance(std::stof(pDistanceAttribute->value.c_str()));
		}

		return node;
	}

	NodePtr Project::LoadLight(xml::Element* pElement, ScenePtr scene)
	{
		NodePtr node = scene->GetRootNode()->FindChild(L"Light");

		auto it_begin = pElement->attributes.begin();
		auto it_end = pElement->attributes.end();

		LightPtr light = std::static_pointer_cast<Light>(node->GetAttribute());
		collection::Vector<StringW> values;

		for (auto it = it_begin; it != it_end; ++it)
		{
			if (wcscmp(it->name.c_str(), L"color") == 0)
			{
				ParseStringW(it->value, L", ", values);
				if (values.size() != 4)
				{
					return nullptr;
				}

				glm::vec4 color;
				color.r = std::stof(values[0]);
				color.g = std::stof(values[1]);
				color.b = std::stof(values[2]);
				color.a = std::stof(values[3]);

				light->SetColor(color);
			}
		}

		return node;
	}

	NodePtr Project::LoadModel(xml::Element* pElement, LoggerPtr logger, DeviceContextPtr deviceContext, ScenePtr scene, NodePtr parent)
	{
		const xml::Attribute* pNameAttribute = xml::FindAttribute(pElement, L"name");
		if (pNameAttribute == nullptr)
		{
			return false;
		}

		const xml::Attribute* pFilePathAttribute = xml::FindAttribute(pElement, L"filePath");
		if (pFilePathAttribute == nullptr)
		{
			return false;
		}

		wchar_t filePath[1024];
		if (PathCombineW(filePath, m_DirPath.c_str(), pFilePathAttribute->value.c_str()) == nullptr)
		{
			return false;
		}

		SkeletalModelPtr model = SkeletalModel::Create(deviceContext);
		if (model->Load(logger, filePath) == false)
		{
			return false;
		}

		SkeletalModel::Finish(logger, model);

		return scene->AddNode(parent, pNameAttribute->value.c_str(), Project::SceneModelGroup, model);
	}

	bool Project::SaveRoot(xml::Element* pDocument, LoggerPtr logger, ScenePtr scene)
	{
		xml::Element* pElement = xml::Element::Create();
		if (pElement != nullptr)
		{
			pElement->name = L"v3dEditor";
			if (xml::AddAttribute(pElement, L"version", Project::XmlV3DEditorVersion) == true)
			{
				pDocument->childs.push_back(pElement);
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}

		if (SaveScene(pElement, logger, scene) == false)
		{
			return false;
		}

		return true;
	}

	bool Project::SaveScene(xml::Element* pParent, LoggerPtr logger, ScenePtr scene)
	{
		xml::Element* pEelement = xml::AddChild(pParent, L"scene");
		if (pEelement != nullptr)
		{
			if (xml::AddAttribute(pEelement, L"version", Project::XmlSceneVersion) == false)
			{
				return false;
			}
		}
		else
		{
			return false;
		}

		if (SaveGrid(pEelement, scene->GetGrid()) == false)
		{
			return false;
		}

		if (SaveSsao(pEelement, scene->GetSsao()) == false)
		{
			return false;
		}

		if (SaveShadow(pEelement, scene->GetShadow()) == false)
		{
			return false;
		}

		if (SaveGlare(pEelement, scene->GetGlare()) == false)
		{
			return false;
		}

		if (SaveImageEffect(pEelement, scene->GetImageEffect()) == false)
		{
			return false;
		}

		if (SaveMisc(pEelement, scene) == false)
		{
			return false;
		}

		if (SaveNode(pEelement, logger, scene->GetRootNode()) == false)
		{
			return false;
		}

		return true;
	}

	bool Project::SaveGrid(xml::Element* pParent, const Scene::Grid& grid)
	{
		xml::Element* pEelement = xml::AddChild(pParent, L"grid");
		if (pEelement == nullptr)
		{
			return false;
		}

		if (xml::AddAttribute(pEelement, L"enable", std::to_wstring(grid.enable).c_str()) == false)
		{
			return false;
		}

		if (xml::AddAttribute(pEelement, L"cellSize", std::to_wstring(grid.cellSize).c_str()) == false)
		{
			return false;
		}

		if (xml::AddAttribute(pEelement, L"halfCellCount", std::to_wstring(grid.halfCellCount).c_str()) == false)
		{
			return false;
		}

		return true;
	}

	bool Project::SaveSsao(xml::Element* pParent, const Scene::Ssao& ssao)
	{
		xml::Element* pEelement = xml::AddChild(pParent, L"ssao");
		if (pEelement == nullptr)
		{
			return false;
		}

		if (xml::AddAttribute(pEelement, L"enable", std::to_wstring(ssao.enable).c_str()) == false)
		{
			return false;
		}

		if (xml::AddAttribute(pEelement, L"radius", std::to_wstring(ssao.radius).c_str()) == false)
		{
			return false;
		}

		if (xml::AddAttribute(pEelement, L"threshold", std::to_wstring(ssao.threshold).c_str()) == false)
		{
			return false;
		}

		if (xml::AddAttribute(pEelement, L"depth", std::to_wstring(ssao.depth).c_str()) == false)
		{
			return false;
		}

		if (xml::AddAttribute(pEelement, L"intensity", std::to_wstring(ssao.intensity).c_str()) == false)
		{
			return false;
		}

		if (xml::AddAttribute(pEelement, L"blurRadius", std::to_wstring(ssao.blurRadius).c_str()) == false)
		{
			return false;
		}

		if (xml::AddAttribute(pEelement, L"blurSamples", std::to_wstring(ssao.blurSamples).c_str()) == false)
		{
			return false;
		}

		return true;
	}

	bool Project::SaveShadow(xml::Element* pParent, const Scene::Shadow& shadow)
	{
		xml::Element* pEelement = xml::AddChild(pParent, L"shadow");
		if (pEelement == nullptr)
		{
			return false;
		}

		if (xml::AddAttribute(pEelement, L"enable", std::to_wstring(shadow.enable).c_str()) == false)
		{
			return false;
		}

		if (xml::AddAttribute(pEelement, L"resolution", std::to_wstring(shadow.resolution).c_str()) == false)
		{
			return false;
		}

		if (xml::AddAttribute(pEelement, L"range", std::to_wstring(shadow.range).c_str()) == false)
		{
			return false;
		}

		if (xml::AddAttribute(pEelement, L"fadeMargin", std::to_wstring(shadow.fadeMargin).c_str()) == false)
		{
			return false;
		}

		if (xml::AddAttribute(pEelement, L"dencity", std::to_wstring(shadow.dencity).c_str()) == false)
		{
			return false;
		}

		if (xml::AddAttribute(pEelement, L"centerOffset", std::to_wstring(shadow.centerOffset).c_str()) == false)
		{
			return false;
		}

		if (xml::AddAttribute(pEelement, L"boundsBias", std::to_wstring(shadow.boundsBias).c_str()) == false)
		{
			return false;
		}

		if (xml::AddAttribute(pEelement, L"depthBias", std::to_wstring(shadow.depthBias).c_str()) == false)
		{
			return false;
		}

		return true;
	}

	bool Project::SaveGlare(xml::Element* pParent, const Scene::Glare& glare)
	{
		xml::Element* pEelement = xml::AddChild(pParent, L"glare");
		if (pEelement == nullptr)
		{
			return false;
		}

		if (xml::AddAttribute(pEelement, L"enable", std::to_wstring(glare.enable).c_str()) == false)
		{
			return false;
		}

		if (xml::AddAttribute(pEelement, L"bpThreshold", std::to_wstring(glare.bpThreshold).c_str()) == false)
		{
			return false;
		}

		if (xml::AddAttribute(pEelement, L"bpOffset", std::to_wstring(glare.bpOffset).c_str()) == false)
		{
			return false;
		}

		if (xml::AddAttribute(pEelement, L"blurRadius", std::to_wstring(glare.blurRadius).c_str()) == false)
		{
			return false;
		}

		if (xml::AddAttribute(pEelement, L"blurSamples", std::to_wstring(glare.blurSamples).c_str()) == false)
		{
			return false;
		}

		if (xml::AddAttribute(pEelement, L"intensity0", std::to_wstring(glare.intensity[0]).c_str()) == false)
		{
			return false;
		}

		if (xml::AddAttribute(pEelement, L"intensity1", std::to_wstring(glare.intensity[1]).c_str()) == false)
		{
			return false;
		}

		return true;
	}

	bool Project::SaveImageEffect(xml::Element* pParent, const Scene::ImageEffect& imageEffect)
	{
		xml::Element* pEelement = xml::AddChild(pParent, L"imageEffect");
		if (pEelement == nullptr)
		{
			return false;
		}

		if (xml::AddAttribute(pEelement, L"toneMappingEnable", std::to_wstring(imageEffect.toneMappingEnable).c_str()) == false)
		{
			return false;
		}

		if (xml::AddAttribute(pEelement, L"fxaaEnable", std::to_wstring(imageEffect.fxaaEnable).c_str()) == false)
		{
			return false;
		}

		return true;
	}

	bool Project::SaveMisc(xml::Element* pParent, ScenePtr scene)
	{
		xml::Element* pMiscEelement = xml::AddChild(pParent, L"misc");
		if (pMiscEelement == nullptr)
		{
			return false;
		}

		// background
		{
			xml::Element* pEelement = xml::AddChild(pMiscEelement, L"background");
			if (pMiscEelement == nullptr)
			{
				return false;
			}

			const glm::vec4& color = scene->GetBackgroundColor();
			StringW temp = std::to_wstring(color.r) + L", " + std::to_wstring(color.g) + L", " + std::to_wstring(color.b) + L", " + std::to_wstring(color.a);
			if (xml::AddAttribute(pEelement, L"color", temp.c_str()) == false)
			{
				return false;
			}
		}

		// select
		{
			xml::Element* pEelement = xml::AddChild(pMiscEelement, L"select");
			if (pMiscEelement == nullptr)
			{
				return false;
			}

			const glm::vec4& color = scene->GetSelectColor();
			StringW temp = std::to_wstring(color.r) + L", " + std::to_wstring(color.g) + L", " + std::to_wstring(color.b) + L", " + std::to_wstring(color.a);
			if (xml::AddAttribute(pEelement, L"color", temp.c_str()) == false)
			{
				return false;
			}
		}

		// display
		for(uint32_t i = 0; i < XmlDisplayCount; i++)
		{
			xml::Element* pEelement = xml::AddChild(pMiscEelement, Project::XmlDisplayNames[i]);
			if (pMiscEelement == nullptr)
			{
				return false;
			}

			bool display = (scene->GetDebugDrawFlags() & Project::XmlDisplayDrawFlags[i]) != 0;
			if (xml::AddAttribute(pEelement, L"enable", std::to_wstring(display).c_str()) == false)
			{
				return false;
			}

			const glm::vec4& color = scene->GetDebugColor(Project::XmlDisplayColorTypes[i]);
			StringW temp = std::to_wstring(color.r) + L", " + std::to_wstring(color.g) + L", " + std::to_wstring(color.b) + L", " + std::to_wstring(color.a);
			if (xml::AddAttribute(pEelement, L"color", temp.c_str()) == false)
			{
				return false;
			}
		}

		return true;
	}

	bool Project::SaveNode(xml::Element* pParent, LoggerPtr logger, const NodePtr node)
	{
		xml::Element* pElement = xml::AddChild(pParent, L"node");
		if (pElement == nullptr)
		{
			return false;
		}

		if (xml::AddAttribute(pElement, L"name", node->GetName()) == false)
		{
			return false;
		}

		const Transform& transform = node->GetLocalTransform();
		StringW temp;

		temp = std::to_wstring(transform.scale.x) + L", " + std::to_wstring(transform.scale.y) + L", " + std::to_wstring(transform.scale.z);
		if (xml::AddAttribute(pElement, L"ls", temp.c_str()) == false)
		{
			return false;
		}

		temp = std::to_wstring(transform.rotation.x) + L", " + std::to_wstring(transform.rotation.y) + L", " + std::to_wstring(transform.rotation.z) + L", " + std::to_wstring(transform.rotation.w);
		if (xml::AddAttribute(pElement, L"lr", temp.c_str()) == false)
		{
			return false;
		}

		temp = std::to_wstring(transform.translation.x) + L", " + std::to_wstring(transform.translation.y) + L", " + std::to_wstring(transform.translation.z);
		if (xml::AddAttribute(pElement, L"lt", temp.c_str()) == false)
		{
			return false;
		}

		NodeAttributePtr nodeAttribute = node->GetAttribute();
		NodeAttribute::TYPE nodeAttributeType = nodeAttribute->GetType();

		switch (nodeAttributeType)
		{
		case NodeAttribute::TYPE_UNDEFINED:
		case NodeAttribute::TYPE_EMPTY:
			SaveAttribute(pElement, nodeAttributeType);
			break;
		case NodeAttribute::TYPE_CAMERA:
			SaveCamera(pElement, std::static_pointer_cast<Camera>(nodeAttribute));
			break;
		case NodeAttribute::TYPE_LIGHT:
			SaveLight(pElement, std::static_pointer_cast<Light>(nodeAttribute));
			break;
		case NodeAttribute::TYPE_MODEL:
			SaveModel(pElement, logger, std::static_pointer_cast<IModel>(nodeAttribute));
			break;

		default:
			return false;
		}

		uint32_t childCount = node->GetChildCount();
		for (uint32_t i = 0; i < childCount; i++)
		{
			if (SaveNode(pElement, logger, node->GetChild(i)) == false)
			{
				return false;
			}
		}

		return true;
	}

	bool Project::SaveAttribute(xml::Element* pElement, NodeAttribute::TYPE type)
	{
		StringW value;

		switch (type)
		{
		case NodeAttribute::TYPE_UNDEFINED:
			value = L"undefined";
			break;
		case NodeAttribute::TYPE_EMPTY:
			value = L"empty";
			break;

		default:
			return false;
		}

		if (xml::AddAttribute(pElement, L"attribute", value.c_str()) == false)
		{
			return false;
		}

		return true;
	}

	bool Project::SaveCamera(xml::Element* pElement, CameraPtr camera)
	{
		if (xml::AddAttribute(pElement, L"attribute", L"camera") == false)
		{
			return false;
		}

		const glm::quat& rotation = camera->GetRotation();
		StringW temp = std::to_wstring(rotation.x) + L", " + std::to_wstring(rotation.y) + L", " + std::to_wstring(rotation.z) + L", " + std::to_wstring(rotation.w);
		if (xml::AddAttribute(pElement, L"rotation", temp.c_str()) == false)
		{
			return false;
		}

		const glm::vec3& at = camera->GetAt();
		temp = std::to_wstring(at.x) + L", " + std::to_wstring(at.y) + L", " + std::to_wstring(at.z);
		if (xml::AddAttribute(pElement, L"at", temp.c_str()) == false)
		{
			return false;
		}

		temp = std::to_wstring(camera->GetDistance());
		if (xml::AddAttribute(pElement, L"distance", temp.c_str()) == false)
		{
			return false;
		}

		return true;
	}

	bool Project::SaveLight(xml::Element* pElement, LightPtr light)
	{
		if (xml::AddAttribute(pElement, L"attribute", L"light") == false)
		{
			return false;
		}

		const glm::vec4& color = light->GetColor();
		StringW temp = std::to_wstring(color.r) + L", " + std::to_wstring(color.g) + L", " + std::to_wstring(color.b) + L", " + std::to_wstring(color.a);
		if (xml::AddAttribute(pElement, L"color", temp.c_str()) == false)
		{
			return false;
		}

		return true;
	}

	bool Project::SaveModel(xml::Element* pElement, LoggerPtr logger, ModelPtr model)
	{
		if (xml::AddAttribute(pElement, L"attribute", L"model") == false)
		{
			return false;
		}

		StringW filePath = model->GetFilePath();
		wchar_t relativeFilePath[1024];

		if (ToRelativePathW(m_DirPath.c_str(), FILE_ATTRIBUTE_DIRECTORY, filePath.c_str(), FILE_ATTRIBUTE_NORMAL, &relativeFilePath[0], sizeof(relativeFilePath) >> 1) == false)
		{
			return false;
		}

		if (xml::AddAttribute(pElement, L"filePath", relativeFilePath) == false)
		{
			return false;
		}

		if (model->Save(logger) == false)
		{
			return false;
		}

		return true;
	}

}
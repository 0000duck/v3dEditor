#include "SamplerFactory.h"

namespace ve {

	/***************************/
	/* public - SamplerFactory */
	/***************************/

	IV3DSampler* SamplerFactory::Create(V3D_FILTER filter, V3D_ADDRESS_MODE addressMode, uint32_t mipLevel, bool anisotropyEnable)
	{
		const V3DDeviceCaps& deviceCaps = m_pNativeDevice->GetCaps();

		SamplerFactory::Key key;
		key.filter = filter;
		key.addressMode = addressMode;
		key.mipLevel = mipLevel;

		auto it = m_Map.find(key);
		if (it != m_Map.end())
		{
			it->second->AddRef();
			return it->second;
		}

		if (deviceCaps.maxSamplerCreateCount == m_pNativeDevice->GetStatistics().samplerCount)
		{
			VE_ASSERT(0);
			return nullptr;
		}

		V3DSamplerDesc samplerDesc;

		switch (filter)
		{
		case V3D_FILTER_NEAREST:
			samplerDesc.magFilter = V3D_FILTER_NEAREST;
			samplerDesc.minFilter = V3D_FILTER_NEAREST;
			samplerDesc.mipmapMode = V3D_MIPMAP_MODE_NEAREST;
			break;
		case V3D_FILTER_LINEAR:
			samplerDesc.magFilter = V3D_FILTER_LINEAR;
			samplerDesc.minFilter = V3D_FILTER_LINEAR;
			samplerDesc.mipmapMode = V3D_MIPMAP_MODE_LINEAR;
			break;

		default:
			VE_ASSERT(0);
		}

		samplerDesc.addressModeU = addressMode;
		samplerDesc.addressModeV = addressMode;
		samplerDesc.addressModeW = V3D_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerDesc.mipLodBias = 0.0f;
		samplerDesc.anisotropyEnable = (anisotropyEnable == true) ? ((deviceCaps.flags & V3D_CAP_SAMPLER_ANISOTROPY) != 0) : false;
		samplerDesc.maxAnisotropy = (samplerDesc.anisotropyEnable == true) ? deviceCaps.maxSamplerAnisotropy : 0.0f;
		samplerDesc.compareEnable = false;
		samplerDesc.compareOp = V3D_COMPARE_OP_ALWAYS;
		samplerDesc.minLod = 0.0f;
		samplerDesc.maxLod = static_cast<float>(mipLevel);
		samplerDesc.borderColor = V3D_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

		IV3DSampler* pSampler;
		if (m_pNativeDevice->CreateSampler(samplerDesc, &pSampler, VE_INTERFACE_DEBUG_NAME(L"VE_Sampler")) != V3D_OK)
		{
			VE_ASSERT(0);
			return nullptr;
		}

		pSampler->AddRef();
		m_Map.insert(std::pair<SamplerFactory::Key, IV3DSampler*>(key, pSampler));

		return pSampler;
	}

	SamplerFactory* SamplerFactory::Create(IV3DDevice* pNativeDevice)
	{
		return VE_NEW_T(SamplerFactory, pNativeDevice);
	}

	void SamplerFactory::Destroy()
	{
		VE_DELETE_THIS_T(this, SamplerFactory);
	}

	/****************************/
	/* private - SamplerFactory */
	/****************************/

	SamplerFactory::SamplerFactory(IV3DDevice* pNativeDevice) :
		m_pNativeDevice(nullptr)
	{
		pNativeDevice->AddRef();
		m_pNativeDevice = pNativeDevice;
	}

	SamplerFactory::~SamplerFactory()
	{
		if (m_Map.empty() == false)
		{
			auto it_begin = m_Map.begin();
			auto it_end = m_Map.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				it->second->Release();
			}
		}

		VE_RELEASE(m_pNativeDevice);
	}

}
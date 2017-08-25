#pragma once

namespace ve {

	class SamplerFactory final
	{
	public:
		IV3DSampler* Create(V3D_FILTER filter, V3D_ADDRESS_MODE addressMode, uint32_t mipLevel = 0, bool anisotropyEnable = false);

	private:
		struct Key
		{
			V3D_FILTER filter;
			V3D_ADDRESS_MODE addressMode;
			uint32_t mipLevel;

			bool operator == (const SamplerFactory::Key& rhs) const
			{
				if ((filter != rhs.filter) ||
					(addressMode != rhs.addressMode) ||
					(mipLevel != rhs.mipLevel))
				{
					return false;
				}

				return true;
			}

			bool operator != (const SamplerFactory::Key& rhs) const
			{
				return !(operator == (rhs));
			}

			uint32_t operator()(const SamplerFactory::Key& key) const
			{
				uint32_t data0 = 1 << key.filter;
				uint32_t data1 = (1 << key.addressMode) << 8;
				uint32_t data2 = (1 << key.mipLevel) << 16;

				return data0 | data1 | data2;
			}
		};

		IV3DDevice* m_pNativeDevice;
		collection::HashMap<SamplerFactory::Key, IV3DSampler*> m_Map;

		static SamplerFactory* Create(IV3DDevice* pNativeDevice);
		void Destroy();

		SamplerFactory(IV3DDevice* pNativeDevice);
		~SamplerFactory();

		friend class Device;

		VE_DECLARE_ALLOCATOR
	};

}
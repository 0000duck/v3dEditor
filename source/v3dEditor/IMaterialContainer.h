#pragma once

namespace ve {

	class IMaterialContainer
	{
	public:
		virtual ~IMaterialContainer() {}

		virtual uint32_t GetMaterialCount() const = 0;
		virtual MaterialPtr GetMaterial(uint32_t materialIndex) = 0;
	};

}
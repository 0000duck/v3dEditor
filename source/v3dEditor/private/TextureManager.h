#pragma once

#include "Texture.h"

namespace ve {

	class TextureManager
	{
	public:
		TexturePtr Find(const wchar_t* pName, V3DFlags stageMask, V3DFlags accessMask);
		void Add(const wchar_t* pName, V3DFlags stageMask, V3DFlags accessMask, TexturePtr texture);
		void Remove(const wchar_t* pName, V3DFlags stageMask, V3DFlags accessMask);

	private:
		struct Key
		{
			StringW name;
			V3DFlags stageMask;
			V3DFlags accessMask;

			bool operator < (const TextureManager::Key& rhs) const;
		};

		Mutex m_Mutex;

		collection::Map<TextureManager::Key, WeakPtr<Texture>> m_Map;

		static TextureManager* Create();

		TextureManager();
		~TextureManager();

		void Destroy();

		friend class Device;

		VE_DECLARE_ALLOCATOR
	};

}
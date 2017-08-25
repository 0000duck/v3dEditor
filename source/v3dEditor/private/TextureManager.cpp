#include "TextureManager.h"
#include "Texture.h"

namespace ve {

	/*********************************/
	/* private - TextureManager::Key */
	/*********************************/

	bool TextureManager::Key::operator < (const TextureManager::Key& rhs) const
	{
		int32_t compResult = name.compare(rhs.name);
		if (compResult < 0)
		{
			return true;
		}
		else if (compResult > 0)
		{
			return false;
		}

		if (stageMask < rhs.stageMask)
		{
			return true;
		}
		else if (stageMask > rhs.stageMask)
		{
			return false;
		}

		if (accessMask < rhs.accessMask)
		{
			return true;
		}
		else if (accessMask > rhs.accessMask)
		{
			return false;
		}

		return false;
	}

	/***************************/
	/* public - TextureManager */
	/***************************/

	TexturePtr TextureManager::Find(const wchar_t* pName, V3DFlags stageMask, V3DFlags accessMask)
	{
		LockGuard<Mutex> lock(m_Mutex);

		TextureManager::Key key;
		key.name = pName;
		key.stageMask = stageMask;
		key.accessMask = accessMask;

		auto it = m_Map.find(key);
		if (it == m_Map.end())
		{
			return nullptr;
		}

		return it->second.lock();
	}

	void TextureManager::Add(const wchar_t* pName, V3DFlags stageMask, V3DFlags accessMask, TexturePtr texture)
	{
		LockGuard<Mutex> lock(m_Mutex);

		TextureManager::Key key;
		key.name = pName;
		key.stageMask = stageMask;
		key.accessMask = accessMask;

		m_Map[key] = texture;
	}

	void TextureManager::Remove(const wchar_t* pName, V3DFlags stageMask, V3DFlags accessMask)
	{
		LockGuard<Mutex> lock(m_Mutex);

		TextureManager::Key key;
		key.name = pName;
		key.stageMask = stageMask;
		key.accessMask = accessMask;

		auto it = m_Map.find(key);
		if (it != m_Map.end())
		{
			m_Map.erase(it);
		}
	}

	/****************************/
	/* private - TextureManager */
	/****************************/

	TextureManager* TextureManager::Create()
	{
		return VE_NEW_T(TextureManager);
	}

	TextureManager::TextureManager()
	{
	}

	TextureManager::~TextureManager()
	{
		VE_ASSERT(m_Map.size() == 0);
	}

	void TextureManager::Destroy()
	{
		VE_DELETE_THIS_T(this, TextureManager);
	}

}
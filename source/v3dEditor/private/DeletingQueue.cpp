#include "DeletingQueue.h"
#include "DeviceContext.h"
#include "ResourceMemoryManager.h"

namespace ve {

	void DeletingQueue::Add(IV3DDeviceChild* pDeviceChild, ResourceAllocation resourceAllocation)
	{
		LockGuard<Mutex> lock(m_Mutex);

		DeletingQueue::Object obj;
		obj.pDeviceChild = pDeviceChild;
		obj.resourceAllocation = resourceAllocation;

		m_Objects[(m_Current + (m_Count - 1)) % m_Count].push_back(obj);
	}

	DeletingQueue* DeletingQueue::Create(ResourceMemoryManager* pResourceMemoryManager, uint32_t frameCount)
	{
		DeletingQueue* pDeletingQueue = VE_NEW_T(DeletingQueue);
		if (pDeletingQueue == nullptr)
		{
			return nullptr;
		}

		if (pDeletingQueue->Initialize(pResourceMemoryManager, frameCount) == false)
		{
			VE_DELETE_T(pDeletingQueue, DeletingQueue);
			return nullptr;
		}

		return pDeletingQueue;
	}

	void DeletingQueue::Destroy()
	{
		FlushAll();

		VE_DELETE_THIS_T(this, DeletingQueue);
	}

	DeletingQueue::DeletingQueue() :
		m_Current(0),
		m_Count(0)
	{
	}

	DeletingQueue::~DeletingQueue()
	{
#ifdef _DEBUG
		auto it_begin = m_Objects.begin();
		auto it_end = m_Objects.end();
		for (auto it = it_begin; it != it_end; ++it)
		{
			VE_ASSERT(it->size() == 0);
		}
#endif //_DEBUG
	}

	bool DeletingQueue::Initialize(ResourceMemoryManager* pResourceMemoryManager, uint32_t frameCount)
	{
		m_pResourceMemoryManager = pResourceMemoryManager;

		m_Count = frameCount + 1;
		m_Objects.resize(m_Count);

		return true;
	}

	void DeletingQueue::Flush()
	{
		LockGuard<Mutex> lock(m_Mutex);

		auto& objects = m_Objects[m_Current];

		DeletingQueue::Object* pObj = objects.data();
		DeletingQueue::Object* pObjEnd = pObj + objects.size();

		while (pObj != pObjEnd)
		{
			VE_RELEASE(pObj->pDeviceChild);

			if (pObj->resourceAllocation != nullptr)
			{
				m_pResourceMemoryManager->Free(pObj->resourceAllocation);
			}

			pObj++;
		}

		objects.clear();

		m_Current = (m_Current + 1) % m_Count;
	}

	void DeletingQueue::FlushAll()
	{
		LockGuard<Mutex> lock(m_Mutex);

		for (uint32_t i = 0; i < m_Count; i++)
		{
			auto& objects = m_Objects[i];

			DeletingQueue::Object* pObj = objects.data();
			DeletingQueue::Object* pObjEnd = pObj + objects.size();

			while (pObj != pObjEnd)
			{
				VE_RELEASE(pObj->pDeviceChild);

				if (pObj->resourceAllocation != nullptr)
				{
					m_pResourceMemoryManager->Free(pObj->resourceAllocation);
				}

				pObj++;
			}

			objects.clear();
		}
	}

}
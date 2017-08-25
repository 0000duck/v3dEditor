#pragma once

namespace ve {

	class DeviceContext;
	class ResourceMemoryManager;

	class DeletingQueue final
	{
	public:
		void Add(IV3DDeviceChild* pDeviceChild, ResourceAllocation resourceAllocation = nullptr);

	private:
		struct Object
		{
			IV3DDeviceChild* pDeviceChild;
			ResourceAllocation resourceAllocation;
		};

		DeviceContext* m_pDeviceContext;
		ResourceMemoryManager* m_pResourceMemoryManager;

		Mutex m_Mutex;

		size_t m_Count;
		size_t m_Current;
		collection::Vector<collection::Vector<DeletingQueue::Object>> m_Objects;

		static DeletingQueue* Create(ResourceMemoryManager* pResourceMemoryManager, uint32_t frameCount);
		void Destroy();

		DeletingQueue();
		~DeletingQueue();

		bool Initialize(ResourceMemoryManager* pResourceMemoryManager, uint32_t frameCount);

		void Flush();
		void FlushAll();

		friend class DeviceContext;

		VE_DECLARE_ALLOCATOR
	};

	template<class T>
	void DeleteDeviceChild(DeletingQueue* pDeletingQueue, T** ppDeviceChild)
	{
		if ((ppDeviceChild == nullptr) || (*ppDeviceChild == nullptr))
		{
			return;
		}

		pDeletingQueue->Add(*ppDeviceChild);
		*ppDeviceChild = nullptr;
	}

	template<class T>
	void DeleteDeviceChild(DeletingQueue* pDeletingQueue, collection::Vector<T*>& deviceChilds)
	{
		if (deviceChilds.empty() == false)
		{
			auto it_begin = deviceChilds.begin();
			auto it_end = deviceChilds.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				if ((*it) != nullptr)
				{
					pDeletingQueue->Add((*it));
					(*it) = nullptr;
				}
			}
		}
	}

	template<class T>
	void DeleteResource(DeletingQueue* pDeletingQueue, T** ppResource, ResourceAllocation* pResourceAllocation)
	{
		if ((ppResource == nullptr) || (*ppResource == nullptr))
		{
			return;
		}

		pDeletingQueue->Add(*ppResource, *pResourceAllocation);
		*ppResource = nullptr;
		*pResourceAllocation = nullptr;
	}

}
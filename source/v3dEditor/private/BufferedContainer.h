#pragma once

namespace ve {

	namespace collection {

		template<typename T>
		class BufferedContainer final
		{
		public:
			BufferedContainer(size_t initialCount, size_t resizeStep) :
				m_pData(nullptr),
				m_pCurrent(nullptr),
				m_Capacity(0),
				m_Count(0)
			{
				m_Capacity = std::max(size_t(1), initialCount);
				m_ResizeStep = resizeStep;
				m_pData = VE_MALLOC_T(T, m_Capacity);
				VE_ASSERT(m_pData != nullptr);

				m_pCurrent = &m_pData[0];
			}

			~BufferedContainer()
			{
				VE_FREE(m_pData);
			}

			void Clear()
			{
				m_pCurrent = &m_pData[0];
				m_Count = 0;
			}

			T* Add(size_t count = 1)
			{
				VE_ASSERT(count >= 1);

				size_t nextCount = m_Count + count;

				if ((m_ResizeStep > 0) && (m_Capacity < nextCount))
				{
					size_t nextCapacity = (nextCount + m_ResizeStep - 1) / m_ResizeStep * m_ResizeStep;

					T* pTemp = VE_REALLOC_T(m_pData, T, nextCapacity);
					VE_ASSERT(pTemp != nullptr);

					m_pData = pTemp;
					m_Capacity = nextCapacity;

					m_pCurrent = &m_pData[m_Count];
				}

				VE_ASSERT(m_Capacity > m_Count);

				T* pRet = m_pCurrent;

				m_pCurrent += count;
				m_Count = nextCount;

				return pRet;
			}

			size_t GetCount() const
			{
				return m_Count;
			}

			T* GetData() const
			{
				return m_pData;
			}

		private:
			T* m_pData;
			T* m_pCurrent;
			size_t m_Capacity;
			size_t m_ResizeStep;
			size_t m_Count;
		};
	}
}
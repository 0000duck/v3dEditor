#pragma once

namespace ve {

	namespace collection {

		template<typename T>
		class DynamicContainer
		{
		public:
			DynamicContainer(size_t initialCount, size_t resizeStep) :
				m_ppData(nullptr),
				m_ppCurrent(nullptr),
				m_Capacity(0),
				m_Count(0)
			{
				m_Capacity = std::max(size_t(1), initialCount);
				m_ResizeStep = resizeStep;

				m_ppData = VE_MALLOC_T(T*, initialCount);
				VE_ASSERT(m_ppData != nullptr);

				for (size_t i = 0; i < m_Capacity; i++)
				{
					m_ppData[i] = VE_NEW_T(T);
					VE_ASSERT(m_ppData[i] != nullptr);
				}

				m_ppCurrent = &m_ppData[0];
			}

			~DynamicContainer()
			{
				for (size_t i = 0; i < m_Capacity; i++)
				{
					VE_DELETE_T(m_ppData[i], T);
				}

				VE_FREE(m_ppData);
			}

			void Clear()
			{
				m_ppCurrent = &m_ppData[0];
				m_Count = 0;
			}

			T** Add(size_t count = 1)
			{
				VE_ASSERT(count >= 1);

				size_t nextCount = m_Count + count;

				if ((m_ResizeStep > 0) && (m_Capacity < nextCount))
				{
					size_t nextCapacity = (nextCount + m_ResizeStep - 1) / m_ResizeStep * m_ResizeStep;

					T** ppTemp = VE_MALLOC_T(T*, nextCapacity);
					VE_ASSERT(ppTemp != nullptr);

					for (size_t i = 0; i < m_Capacity; i++)
					{
						ppTemp[i] = m_ppData[i];
					}

					VE_FREE(m_ppData);
					m_ppData = ppTemp;

					for (size_t i = m_Capacity; i < nextCapacity; i++)
					{
						m_ppData[i] = VE_NEW_T(T);
						VE_ASSERT(m_ppData[i] != nullptr);
					}

					m_ppCurrent = &m_ppData[m_Count];

					m_Capacity = nextCapacity;
				}

				VE_ASSERT(m_Capacity > m_Count);

				T** ppRet = m_ppCurrent;

				m_ppCurrent += count;
				m_Count = nextCount;

				return ppRet;
			}

			size_t GetCount() const
			{
				return m_Count;
			}

			T** GetData() const
			{
				return m_ppData;
			}

		private:
			T** m_ppData;
			T** m_ppCurrent;
			size_t m_Capacity;
			size_t m_ResizeStep;
			size_t m_Count;
		};
	};

}
#include "pch.h"

// ----------------------------------------------------------------------------------------------------
// メモリ
// ----------------------------------------------------------------------------------------------------

#ifdef _DEBUG
#define VE_DEBUG_MEMORY
#endif //_DEBUG

#if defined(VE_TRACKING_MEMORY) || defined(VE_DEBUG_MEMORY)
#define VE_ENABLE_MEMORY_EXTRA
#endif

namespace ve {

#ifdef VE_ENABLE_MEMORY_EXTRA

	// メモリ情報構造体
	struct MemoryInfo
	{
		size_t allocateCount;
		size_t allocateSize;
		size_t extraSize;
	};

	// メモリアロケート構造体
	struct MemoryAllocate
	{
		size_t count;

#ifdef VE_DEBUG_MEMORY
		std::wstring file;
		size_t line;

		MemoryAllocate(size_t count_, const wchar_t* file_, size_t line_)
		{
			count = count_;
			file = file_;
			line = line_;
		}
#else //VE_DEBUG_MEMORY
		MemoryAllocate(size_t count_)
		{
			count = count_;
		}
#endif //VE_DEBUG_MEMORY
	};

	// メモリアロケートマップ
	typedef std::map<void*, MemoryAllocate> MemoryAllocateMap;

	// メモリカテゴリデータ構造体
	struct MemoryCategoryData
	{
		Mutex mutex;
		ve::MemoryInfo info;
		ve::MemoryAllocateMap allocMap;
	};

	// メモリカテゴリデータ配列
	static MemoryCategoryData* memoryCategoryDats = nullptr;

#endif //RISE_ENABLE_MEMORY_EXTRA

#ifdef VE_DEBUG_MEMORY
	//メモリチェック用の値
	static constexpr uint32_t memoryMagicNumber = 0x45443356; // マジックナンバー ('V' '3' 'D' 'E')
	static constexpr uint32_t memoryBlokenNumber = 0xCDCDCDCD; // メモリ破壊確認用の値

	// Debug
	// マジックナンバー、トラッキング、ギャップが有効になる
	static constexpr uint8_t memoryHeaderSize = 4 * 3;
#else //VE_DEBUG_MEMORY
#ifdef VE_TRACKING_MEMORY
	// Release (メモリのトラッキングが有効 )
	// トラッキング、ギャップ が有効になる
	static constexpr uint8_t memoryHeaderSize = 4 * 2;
#else //VE_TRACKING_MEMORY
	// Release ( メモリのトラッキングが無効 )
	// トラッキング( アライメント、オフセットのみ )、ギャップが有効になる
	static constexpr uint8_t memoryHeaderSize = 2 + 4;
#endif //VE_TRACKING_MEMORY
#endif //VE_DEBUG_MEMORY

	static constexpr uint32_t memoryGapValue = 0x56474D56; // ギャップの値 ('V' 'M' 'G' 'V')

	static constexpr uint8_t defaultMemoryAlignment = static_cast<uint8_t>(sizeof(size_t));
	static constexpr uint8_t defaultMemoryHeadSize = (memoryHeaderSize / defaultMemoryAlignment) * defaultMemoryAlignment + (((memoryHeaderSize % defaultMemoryAlignment) > 0) ? defaultMemoryAlignment : 0);

	// 内部用メモリ解放
	void InternalFreeMemory(void* ptr, size_t count)
	{
		VE_ASSERT(ptr != nullptr);

#ifdef VE_ENABLE_MEMORY_EXTRA

		uint8_t* mem = reinterpret_cast<uint8_t*>(ptr);

#ifdef VE_DEBUG_MEMORY
		//マジックナンバーのチェック
		uint32_t* magicNumber = reinterpret_cast<uint32_t*>(&mem[-12]);
		VE_ASSERT(*magicNumber == ve::memoryMagicNumber);
#endif //_DEBUG

		uint8_t offset = mem[-5];

#ifdef VE_DEBUG_MEMORY
		//メモリ破壊のチェック
		uint32_t* blokenNumber = reinterpret_cast<uint32_t*>(&mem[count]);
		VE_ASSERT(*blokenNumber == ve::memoryBlokenNumber);
#endif //VE_DEBUG_MEMORY

		mem = mem - offset;

		_aligned_free(mem);

#else //VE_ENABLE_MEMORY_EXTRA
		uint8_t* mem = reinterpret_cast<uint8_t*>(ptr);
		uint8_t offset = mem[-5];

		mem = mem - offset;

		_aligned_free(mem);
#endif //VE_ENABLE_MEMORY_EXTRA
	}

	void* AllocateMemory(size_t count, uint8_t category, const wchar_t* file, size_t line)
	{
		VE_ASSERT(count > 0);

#ifdef VE_ENABLE_MEMORY_EXTRA
		// ----------------------------------------------------------------------------------------------------
		// メモリを確保
		// ----------------------------------------------------------------------------------------------------

		size_t bytes = count;

		bytes += ve::defaultMemoryHeadSize; // Head ヘッダを書き込むサイズ
#ifdef VE_DEBUG_MEMORY
		bytes += ve::defaultMemoryAlignment; // Tail メモリ破格確認用の値を書き込むサイズ
#endif //VE_DEBUG_MEMORY

		uint8_t* mem = reinterpret_cast<uint8_t*>(_aligned_malloc(bytes, sizeof(void*)));
		if (mem == nullptr)
		{
			return nullptr;
		}

		// ----------------------------------------------------------------------------------------------------
		// Head
		// ----------------------------------------------------------------------------------------------------

#ifdef VE_DEBUG_MEMORY
		::memset(mem, 0, ve::defaultMemoryHeadSize);
#endif //VE_DEBUG_MEMORY

		mem += ve::defaultMemoryHeadSize;

#ifdef VE_DEBUG_MEMORY
		// マジックナンバー
		uint32_t* magicNumber = reinterpret_cast<uint32_t*>(&mem[-12]);
		*magicNumber = ve::memoryMagicNumber;
#endif //VE_DEBUG_MEMORY

		// 情報
		mem[-7] = category;
		mem[-6] = ve::defaultMemoryAlignment;
		mem[-5] = ve::defaultMemoryHeadSize;

		// ギャップ
		uint32_t* gapValue = reinterpret_cast<uint32_t*>(&mem[-4]);
		*gapValue = ve::memoryGapValue;

		// ----------------------------------------------------------------------------------------------------
		// Tail
		// ----------------------------------------------------------------------------------------------------

#ifdef VE_DEBUG_MEMORY
		::memset(&mem[count], 0, ve::defaultMemoryAlignment);

		//メモリ破壊の確認用
		uint32_t* blokenNumber = reinterpret_cast<uint32_t*>(&mem[count]);
		*blokenNumber = ve::memoryBlokenNumber;
#endif //VE_DEBUG_MEMORY

		// ----------------------------------------------------------------------------------------------------
		// メモリ追跡用情報
		// ----------------------------------------------------------------------------------------------------

		ve::MemoryCategoryData& data = ve::memoryCategoryDats[category];

		data.mutex.lock();

		data.info.allocateCount += 1;
		data.info.allocateSize += bytes;
		data.info.extraSize += bytes - count;
#ifdef VE_DEBUG_MEMORY
		data.allocMap.insert(ve::MemoryAllocateMap::value_type(mem, ve::MemoryAllocate(count, file, line)));
#else //VE_DEBUG_MEMORY
		data.allocMap.insert(ve::MemoryAllocateMap::value_type(mem, ve::MemoryAllocate(count)));
#endif //VE_DEBUG_MEMORY

		data.mutex.unlock();

		// ----------------------------------------------------------------------------------------------------

		return mem;

#else //VE_ENABLE_MEMORY_EXTRA
		// ----------------------------------------------------------------------------------------------------
		// メモリを確保
		// ----------------------------------------------------------------------------------------------------

		size_t bytes = count + ve::defaultMemoryHeadSize;
		uint8_t* mem = reinterpret_cast<uint8_t*>(_aligned_malloc(bytes, sizeof(void*)));
		if (mem == nullptr)
		{
			return nullptr;
		}

		// ----------------------------------------------------------------------------------------------------
		// Head
		// ----------------------------------------------------------------------------------------------------

		mem += ve::defaultMemoryHeadSize;

		// 情報
		mem[-6] = ve::defaultMemoryAlignment;
		mem[-5] = ve::defaultMemoryHeadSize;

		// ギャップ
		uint32_t* gapValue = reinterpret_cast<uint32_t*>(&mem[-4]);
		*gapValue = ve::memoryGapValue;

		// ----------------------------------------------------------------------------------------------------

		return mem;
#endif //VE_ENABLE_MEMORY_EXTRA
	}

	void* AllocateAlignedMemory(size_t align, size_t count, uint8_t category, const wchar_t* file, size_t line)
	{
		VE_ASSERT(4 <= align && align <= 128);
		VE_ASSERT(count > 0);

#ifdef VE_ENABLE_MEMORY_EXTRA
		// ----------------------------------------------------------------------------------------------------
		// メモリを確保
		// ----------------------------------------------------------------------------------------------------

		size_t bytes = count;
		size_t headBytes = (memoryHeaderSize / align) * align + (((memoryHeaderSize % align) > 0) ? align : 0);
		const size_t& tailBytes = align;

		VE_ASSERT(headBytes < 255);
		bytes += headBytes;

#ifdef VE_DEBUG_MEMORY
		bytes += tailBytes;
#endif //VE_DEBUG_MEMORY

		uint8_t* mem = reinterpret_cast<uint8_t*>(_aligned_malloc(bytes, align));
		if (mem == nullptr)
		{
			return nullptr;
		}

		// ----------------------------------------------------------------------------------------------------
		// Head
		// ----------------------------------------------------------------------------------------------------

#ifdef VE_DEBUG_MEMORY
		::memset(mem, 0, headBytes);
#endif //VE_DEBUG_MEMORY

		mem += headBytes;

#ifdef VE_DEBUG_MEMORY
		// マジックナンバー
		uint32_t* magicNumber = reinterpret_cast<uint32_t*>(&mem[-12]);
		*magicNumber = ve::memoryMagicNumber;
#endif //VE_DEBUG_MEMORY

		// 情報
		mem[-7] = category;
		mem[-6] = static_cast<uint8_t>(align);
		mem[-5] = static_cast<uint8_t>(headBytes);

		// ギャップ
		uint32_t* gapValue = reinterpret_cast<uint32_t*>(&mem[-4]);
		*gapValue = ve::memoryGapValue;

		// ----------------------------------------------------------------------------------------------------
		// Tail
		// ----------------------------------------------------------------------------------------------------

#ifdef VE_DEBUG_MEMORY
		::memset(&mem[count], 0, tailBytes);

		//メモリ破壊の確認用
		uint32_t* blokenNumber = reinterpret_cast<uint32_t*>(&mem[count]);
		*blokenNumber = ve::memoryBlokenNumber;
#endif //VE_DEBUG_MEMORY

		// ----------------------------------------------------------------------------------------------------
		// メモリ追跡用情報
		// ----------------------------------------------------------------------------------------------------

		ve::MemoryCategoryData& data = ve::memoryCategoryDats[category];

		data.mutex.lock();

		data.info.allocateCount += 1;
		data.info.allocateSize += bytes;
		data.info.extraSize += bytes - count;
#ifdef VE_DEBUG_MEMORY
		data.allocMap.insert(ve::MemoryAllocateMap::value_type(mem, ve::MemoryAllocate(count, file, line)));
#else //VE_DEBUG_MEMORY
		data.allocMap.insert(ve::MemoryAllocateMap::value_type(mem, ve::MemoryAllocate(count)));
#endif //VE_DEBUG_MEMORY

		data.mutex.unlock();

		//////////////////////////////////////////////////

		return mem;

#else //VE_ENABLE_MEMORY_EXTRA
		// ----------------------------------------------------------------------------------------------------
		// メモリを確保
		// ----------------------------------------------------------------------------------------------------

		size_t headBytes = (ve::memoryHeaderSize / align) * align + (((ve::memoryHeaderSize % align) > 0) ? align : 0);
		size_t bytes = count + headBytes;

		uint8_t* mem = reinterpret_cast<uint8_t*>(_aligned_malloc(bytes, align));
		if (mem == nullptr)
		{
			return nullptr;
		}

		// ----------------------------------------------------------------------------------------------------
		// Head
		// ----------------------------------------------------------------------------------------------------

		mem += ve::defaultMemoryHeadSize;

		// 情報
		mem[-6] = static_cast<uint8_t>(align);
		mem[-5] = static_cast<uint8_t>(headBytes);

		// ギャップ
		uint32_t* gapValue = reinterpret_cast<uint32_t*>(&mem[-4]);
		*gapValue = ve::memoryGapValue;

		// ----------------------------------------------------------------------------------------------------

		return mem;
#endif //VE_ENABLE_MEMORY_EXTRA
	}

	void* ReallocMemory(void* ptr, size_t count, const wchar_t* file, size_t line)
	{
		VE_ASSERT(ptr != nullptr);
		VE_ASSERT(count > 0);

#ifdef VE_ENABLE_MEMORY_EXTRA

		uint8_t* mem = reinterpret_cast<uint8_t*>(ptr);

#ifdef VE_DEBUG_MEMORY
		// ----------------------------------------------------------------------------------------------------
		// マジックナンバーのチェック
		// ----------------------------------------------------------------------------------------------------

		uint32_t* magicNumber = reinterpret_cast<uint32_t*>(&mem[-12]);
		if (*magicNumber != ve::memoryMagicNumber)
		{
			VE_ASSERT(0);
		}

		// ----------------------------------------------------------------------------------------------------
		// ギャップのチェック
		// ----------------------------------------------------------------------------------------------------

		uint32_t* gapValue = reinterpret_cast<uint32_t*>(&mem[-4]);
		if (*gapValue != ve::memoryGapValue)
		{
			VE_ASSERT(0);
		}
#endif //VE_DEBUG_MEMORY

		uint8_t category = mem[-7];
		uint8_t align = mem[-6];
		uint8_t offset = mem[-5];
		size_t bytes = count + offset + align;

		// ----------------------------------------------------------------------------------------------------
		// 以前のメモリ追跡用情報をバックアップして削除
		// ----------------------------------------------------------------------------------------------------

		ve::MemoryCategoryData& data = ve::memoryCategoryDats[category];

		data.mutex.lock();

		ve::MemoryAllocateMap::iterator it_alloc = data.allocMap.find(ptr);
		VE_ASSERT(it_alloc != data.allocMap.end());
		ve::MemoryAllocate alloc = it_alloc->second;
		data.info.allocateSize -= alloc.count + offset + align;
		data.allocMap.erase(it_alloc);

		data.mutex.unlock();

		// ----------------------------------------------------------------------------------------------------
		// メモリを再確保
		// ----------------------------------------------------------------------------------------------------

		mem = mem - offset;

		void* temp = _aligned_realloc(mem, bytes, sizeof(void*));
		if (temp == nullptr)
		{
			//allocateSize は事前に引かれている
			//MemoryAllocate はすでに削除済みなので、internalFreeMemory では allocMap からの削除は行わない
			data.mutex.lock();
			data.info.allocateCount -= 1;
			data.info.extraSize -= offset + align;
			data.mutex.unlock();

			InternalFreeMemory(ptr, alloc.count);
			return nullptr;
		}

		mem = reinterpret_cast<uint8_t*>(temp);

		// ----------------------------------------------------------------------------------------------------
		// Head
		// ----------------------------------------------------------------------------------------------------

		mem += offset; // ヘッダはスキップ

		// ----------------------------------------------------------------------------------------------------
		// Tail
		// ----------------------------------------------------------------------------------------------------

#ifdef VE_DEBUG_MEMORY
		::memset(&mem[count], 0, align);

		//メモリ破壊の確認用
		uint32_t* blokenNumber = reinterpret_cast<uint32_t*>(&mem[count]);
		*blokenNumber = ve::memoryBlokenNumber;
#endif //VE_DEBUG_MEMORY

		// ----------------------------------------------------------------------------------------------------
		// メモリ追跡用情報を変更
		// ----------------------------------------------------------------------------------------------------

		data.mutex.lock();

		data.info.allocateSize += bytes;
		alloc.count = count;
		data.allocMap.insert(ve::MemoryAllocateMap::value_type(mem, alloc));

		data.mutex.unlock();

		// ----------------------------------------------------------------------------------------------------

		return mem;

#else //VE_ENABLE_MEMORY_EXTRA
		uint8_t* mem = reinterpret_cast<uint8_t*>(ptr);
		uint8_t align = mem[-6];
		uint8_t offset = mem[-5];
		size_t bytes = count + offset + align;

		mem = mem - offset;

		void* temp = _aligned_realloc(mem, bytes, align);
		if (temp == nullptr)
		{
			return nullptr;
		}

		mem = reinterpret_cast<uint8_t*>(temp);
		mem = mem + offset;

		return mem;
#endif //VE_ENABLE_MEMORY_EXTRA
	}

	void* ReallocAlignedMemory(void* ptr, size_t align, size_t count, const wchar_t* file, size_t line)
	{
		VE_ASSERT(ptr != nullptr);
		VE_ASSERT(count > 0);

#ifdef VE_ENABLE_MEMORY_EXTRA

		uint8_t* mem = reinterpret_cast<uint8_t*>(ptr);

#ifdef VE_DEBUG_MEMORY
		// ----------------------------------------------------------------------------------------------------
		// マジックナンバーのチェック
		// ----------------------------------------------------------------------------------------------------

		uint32_t* magicNumber = reinterpret_cast<uint32_t*>(&mem[-12]);
		if (*magicNumber != ve::memoryMagicNumber)
		{
			VE_ASSERT(0);
		}

		// ----------------------------------------------------------------------------------------------------
		// ギャップのチェック
		// ----------------------------------------------------------------------------------------------------

		uint32_t* gapValue = reinterpret_cast<uint32_t*>(&mem[-4]);
		if (*gapValue != ve::memoryGapValue)
		{
			VE_ASSERT(0);
		}
#endif //VE_DEBUG_MEMORY

		uint8_t category = mem[-7];
		uint8_t offset = mem[-5];

		mem[-6] = static_cast<uint8_t>(align);

		size_t bytes = count + offset + align;

		// ----------------------------------------------------------------------------------------------------
		// 以前のメモリ追跡用情報をバックアップして削除
		// ----------------------------------------------------------------------------------------------------

		ve::MemoryCategoryData& data = ve::memoryCategoryDats[category];

		data.mutex.lock();

		ve::MemoryAllocateMap::iterator it_alloc = data.allocMap.find(ptr);
		VE_ASSERT(it_alloc != data.allocMap.end());
		ve::MemoryAllocate alloc = it_alloc->second;
		//		data.info.allocateSize -= alloc.count + offset + align;
		data.allocMap.erase(it_alloc);

		data.mutex.unlock();

		// ----------------------------------------------------------------------------------------------------
		// メモリを再確保
		// ----------------------------------------------------------------------------------------------------

		mem = mem - offset;

		void* temp = _aligned_realloc(mem, bytes, align);
		if (temp == nullptr)
		{
			//allocateSize は事前に引かれている
			//MemoryAllocate はすでに削除済みなので、internalFreeMemory では allocMap からの削除は行わない
			data.mutex.lock();
			data.info.allocateCount -= 1;
			data.info.extraSize -= offset + align;
			data.mutex.unlock();

			InternalFreeMemory(ptr, alloc.count);
			return nullptr;
		}

		mem = reinterpret_cast<uint8_t*>(temp);

		// ----------------------------------------------------------------------------------------------------
		// Head
		// ----------------------------------------------------------------------------------------------------

		mem += offset; // ヘッダはスキップ

		// ----------------------------------------------------------------------------------------------------
		// Tail
		// ----------------------------------------------------------------------------------------------------

#ifdef VE_DEBUG_MEMORY
		::memset(&mem[count], 0, align);

		//メモリ破壊の確認用
		uint32_t* blokenNumber = reinterpret_cast<uint32_t*>(&mem[count]);
		*blokenNumber = ve::memoryBlokenNumber;
#endif //VE_DEBUG_MEMORY

		// ----------------------------------------------------------------------------------------------------
		// メモリ追跡用情報を変更
		// ----------------------------------------------------------------------------------------------------

		data.mutex.lock();

		data.info.allocateSize += bytes;
		alloc.count = count;
		data.allocMap.insert(ve::MemoryAllocateMap::value_type(mem, alloc));

		data.mutex.unlock();

		// ----------------------------------------------------------------------------------------------------

		return mem;

#else //VE_ENABLE_MEMORY_EXTRA
		uint8_t* mem = reinterpret_cast<uint8_t*>(ptr);
		uint8_t offset = mem[-5];

		mem[-6] = static_cast<uint8_t>(align);

		size_t bytes = count + offset + align;

		mem = mem - offset;

		void* temp = _aligned_realloc(mem, bytes, align);
		if (temp == nullptr)
		{
			return nullptr;
		}

		mem = reinterpret_cast<uint8_t*>(temp);
		mem = mem + offset;

		return mem;
#endif //VE_ENABLE_MEMORY_EXTRA
	}

	void FreeMemory(void* ptr, const wchar_t* file, size_t line)
	{
		if (ptr == nullptr)
		{
			return;
		}

#ifdef VE_ENABLE_MEMORY_EXTRA

		uint8_t* mem = reinterpret_cast<uint8_t*>(ptr);

#ifdef VE_DEBUG_MEMORY
		// ----------------------------------------------------------------------------------------------------
		// マジックナンバーのチェック
		// ----------------------------------------------------------------------------------------------------

		uint32_t* magicNumber = reinterpret_cast<uint32_t*>(&mem[-12]);
		if (*magicNumber != ve::memoryMagicNumber)
		{
			VE_ASSERT(0);
		}

		// ----------------------------------------------------------------------------------------------------
		// ギャップのチェック
		// ----------------------------------------------------------------------------------------------------

		uint32_t* gapValue = reinterpret_cast<uint32_t*>(&mem[-4]);
		if (*gapValue != ve::memoryGapValue)
		{
			VE_ASSERT(0);
		}
#endif //VE_DEBUG_MEMORY

		// ----------------------------------------------------------------------------------------------------
		// メモリ追跡用情報の削除
		// ----------------------------------------------------------------------------------------------------

		uint8_t category = mem[-7];
		ve::MemoryCategoryData& data = ve::memoryCategoryDats[category];

		data.mutex.lock();

		ve::MemoryAllocateMap::iterator it_alloc = data.allocMap.find(ptr);
		VE_ASSERT(it_alloc != data.allocMap.end());

#ifdef VE_DEBUG_MEMORY
		//メモリ破壊のチェック
		uint32_t* blokenNumber = reinterpret_cast<uint32_t*>(&mem[it_alloc->second.count]);
		if (*blokenNumber != ve::memoryBlokenNumber)
		{
			VE_ASSERT(0);
		}
#endif //VE_DEBUG_MEMORY

		uint8_t align = mem[-6];
		uint8_t offset = mem[-5];

		// 追跡用情報の削除
		data.info.allocateCount -= 1;
		data.info.allocateSize -= it_alloc->second.count + offset + align;
		data.info.extraSize -= offset + align;
		data.allocMap.erase(it_alloc);

		data.mutex.unlock();

		// ----------------------------------------------------------------------------------------------------

		mem = mem - offset;

		_aligned_free(mem);

#else //VE_ENABLE_MEMORY_EXTRA
		uint8_t* mem = reinterpret_cast<uint8_t*>(ptr);
		uint8_t offset = mem[-5];

		mem = mem - offset;

		_aligned_free(mem);
#endif //VE_ENABLE_MEMORY_EXTRA
	}

	void Initialize()
	{
#ifdef VE_ENABLE_MEMORY_EXTRA
		// ----------------------------------------------------------------------------------------------------
		//メモリトラッキングの初期化処理
		// ----------------------------------------------------------------------------------------------------

		ve::memoryCategoryDats = new ve::MemoryCategoryData[ve::MEMORY_TYPE_COUNT];
		for (uint32_t i = 0; i < ve::MEMORY_TYPE_COUNT; i++)
		{
			ve::memoryCategoryDats[i].info.allocateCount = 0;
			ve::memoryCategoryDats[i].info.allocateSize = 0;
			ve::memoryCategoryDats[i].info.extraSize = 0;
		}
#endif //VE_ENABLE_MEMORY_EXTRA
	}

	void Finalize()
	{
#ifdef VE_ENABLE_MEMORY_EXTRA

		// ----------------------------------------------------------------------------------------------------
		// メモリトラッキングの終了処理
		// ----------------------------------------------------------------------------------------------------

		constexpr wchar_t* memCategories[ve::MEMORY_TYPE_COUNT] =
		{
			L"General",
		};

		for (uint32_t i = 0; i < ve::MEMORY_TYPE_COUNT; i++)
		{
			ve::MemoryCategoryData& data = ve::memoryCategoryDats[i];

			if (data.allocMap.size() > 0)
			{
				wchar_t temp[2048];

				auto it_alloc_begin = data.allocMap.begin();
				auto it_alloc_end = data.allocMap.end();

				wchar_t categoryName[64];
				::wcscpy_s(categoryName, memCategories[i]);

				wsprintf(temp, L"!!! MemoryLeak : %s : AllocateSize[%u] AllocateCount[%u]\n", memCategories[i], data.info.allocateSize, data.info.allocateCount);
				OutputDebugString(temp);

				for (auto it_alloc = it_alloc_begin; it_alloc != it_alloc_end; ++it_alloc)
				{
					uint8_t* mem = reinterpret_cast<uint8_t*>(it_alloc->first);
					const ve::MemoryAllocate& alloc = it_alloc->second;

					wsprintf(temp, L"  %s(%u) : size(%u byte)\n", alloc.file.c_str(), alloc.line, alloc.count);
					OutputDebugString(temp);

					ve::InternalFreeMemory(mem, alloc.count);
				}
			}
		}

		delete[] ve::memoryCategoryDats;

#endif //VE_ENABLE_MEMORY_EXTRA
	}

}

// ----------------------------------------------------------------------------------------------------
// その他
// ----------------------------------------------------------------------------------------------------

#ifdef _DEBUG

uint32_t ve::U64ToU32(uint64_t value)
{
	VE_ASSERT(UINT32_MAX >= value);

	return static_cast<uint32_t>(value);
}

#endif //_DEBUG

uint32_t ve::ToMagicNumber(char a, char b, char c, char d)
{
	uint32_t ua = a;
	uint32_t ub = b << 8;
	uint32_t uc = c << 16;
	uint32_t ud = d << 24;

	return ud | uc | ub | ua;
}

const char* ve::ToString_ResourceType(V3D_RESOURCE_TYPE type)
{
	static constexpr char* strings[2]
	{
		"Buffer",
		"Image",
	};

	VE_ASSERT(_countof(strings) > type);

	return strings[type];
}

void ve::ToString_MemoryProperty(V3DFlags flags, StringA& string)
{
	static constexpr V3D_MEMORY_PROPERTY_FLAG flagBits[] =
	{
		V3D_MEMORY_PROPERTY_DEVICE_LOCAL,
		V3D_MEMORY_PROPERTY_HOST_VISIBLE,
		V3D_MEMORY_PROPERTY_HOST_COHERENT,
		V3D_MEMORY_PROPERTY_HOST_CACHED,
		V3D_MEMORY_PROPERTY_LAZILY_ALLOCATED,
	};

	static constexpr char* stringTable[] =
	{
		"DEVICE_LOCAL",
		"HOST_VISIBLE",
		"HOST_COHERENT",
		"HOST_CACHED",
		"LAZILY_ALLOCATED",
	};

	VE_ASSERT(_countof(flagBits) ==_countof(stringTable));

	bool first = true;

	for (auto i = 0; i < _countof(flagBits); i++)
	{
		if (flags & flagBits[i])
		{
			if (first == false)
			{
				string += " | ";
			}
			else
			{
				first = false;
			}

			string += stringTable[i];
		}
	}
}

void ve::ToMultibyteString(const wchar_t* pSrc, ve::StringA& dst)
{
	size_t bufferSize = 0;

	wcstombs_s(&bufferSize, nullptr, 0, pSrc, _TRUNCATE);

	char* pBuffer = VE_MALLOC_T(char, (bufferSize + 1));

	wcstombs_s(&bufferSize, pBuffer, bufferSize + 1, pSrc, _TRUNCATE);
	pBuffer[bufferSize] = L'\0';

	dst = pBuffer;

	VE_FREE(pBuffer);
}

void ve::ToWideString(const char* pSrc, StringW& dst)
{
	size_t bufferSize = 0;

	mbstowcs_s(&bufferSize, nullptr, 0, pSrc, _TRUNCATE);

	wchar_t* pBuffer = VE_MALLOC_T(wchar_t, bufferSize + 1);

	mbstowcs_s(&bufferSize, pBuffer, bufferSize, pSrc, _TRUNCATE);
	pBuffer[bufferSize - 1] = L'\0';

	dst = pBuffer;

	VE_FREE(pBuffer);
}

void ve::ParseStringW(const StringW& source, const wchar_t* pDelimiters, collection::Vector<StringW>& items)
{
	size_t srcSize = source.size();
	size_t delimSize = wcslen(pDelimiters);
	size_t pos = 0;

	while (pos < srcSize)
	{
		for (size_t i = pos; i < srcSize; i++)
		{
			bool findDelim = false;
			size_t j;

			for (j = 0; (j < delimSize) && (findDelim == false); j++)
			{
				if (source[i] == pDelimiters[j])
				{
					findDelim = true;
				}
			}

			if (findDelim == true)
			{
				if (i != pos)
				{
					items.push_back(source.substr(pos, i - pos));
				}

				pos = i + 1;

				break;
			}
			else
			{
				if (i == (srcSize - 1))
				{
					items.push_back(source.substr(pos, i - pos + 1));

					pos = srcSize;

					break;
				}
			}
		}
	}
}

void ve::RemoveFileExtensionW(const wchar_t* pSrc, StringW& dst)
{
	int32_t srcSize = static_cast<int32_t>(wcslen(pSrc));

	for (int32_t i = (srcSize - 1); i >= 0; i--)
	{
		if (pSrc[i] == L'.')
		{
			dst = pSrc;
			dst.resize(i);
			dst.shrink_to_fit();
		}
	}

	if (dst.empty() == true)
	{
		dst = pSrc;
	}
}

bool ve::IsRelativePath(const wchar_t* pFilePath)
{
	VE_ASSERT(pFilePath != nullptr);

	if (wcslen(pFilePath) < 3)
	{
		return true;
	}

	return (pFilePath[1] != L':');
}

void ve::GetDirectoryPath(const wchar_t* pFilePath, StringW& directoryPath)
{
	VE_ASSERT(pFilePath != nullptr);

	directoryPath = L"";

	int32_t lastIndex = static_cast<int32_t>(wcslen(pFilePath) - 1);

	for (int32_t i = lastIndex; i >= 0; i--)
	{
		if ((pFilePath[i] == L'\\') || (pFilePath[i] == L'/'))
		{
			directoryPath = pFilePath;

			if (i != lastIndex)
			{
				directoryPath.resize(i + 1);
			}

			break;
		}
	}
}

bool ve::RemoveFileSpecW(const wchar_t* pFilePath, StringW& dirPath)
{
	dirPath = pFilePath;

	if (PathRemoveFileSpecW(&dirPath[0]) == TRUE)
	{
		size_t dirPathLength = wcslen(dirPath.data());
		dirPath.resize(dirPathLength);

		if ((dirPath.back() != L'\\') && (dirPath.back() != L'/'))
		{
			dirPath += L'\\';
		}

		dirPath.shrink_to_fit();
	}
	else
	{
		return false;
	}

	return true;
}

bool ve::ToRelativePathW(const wchar_t* pPathFrom, uint32_t attrFrom, const wchar_t* pPathTo, uint32_t attrTo, wchar_t* pRelativePath, size_t relativePathSize)
{
	StringW temp;
	temp.resize(1024, L'\0');

	if (PathRelativePathToW(&temp[0], pPathFrom, attrFrom, pPathTo, attrTo) == FALSE)
	{
		return false;
	}

	temp.resize(wcslen(temp.data()));
	
	size_t pos = temp.find(L".\\", 0);
	if (pos != StringW::npos)
	{
		temp.erase(0, 2);
	}
	else
	{
		pos = temp.find(L"./", 0);
		if (pos != StringW::npos)
		{
			temp.erase(0, 2);
		}
	}

	if (wcscpy_s(pRelativePath, relativePathSize, temp.data()) != 0)
	{
		return false;
	}

	return true;
}

bool ve::FileRead(HANDLE fileHandle, uint64_t size, void* pData)
{
	DWORD readSize = 0;

	if (size > UINT32_MAX)
	{
		return false;
	}

	if (ReadFile(fileHandle, pData, static_cast<uint32_t>(size), &readSize, nullptr) == FALSE)
	{
		return false;
	}

	if (size != readSize)
	{
		return false;
	}

	return true;
}

bool ve::FileWrite(HANDLE fileHandle, uint64_t size, void* pData)
{
	DWORD writeSize = 0;

	if (size > UINT32_MAX)
	{
		return false;
	}

	if (WriteFile(fileHandle, pData, static_cast<uint32_t>(size), &writeSize, nullptr) == FALSE)
	{
		return false;
	}

	if (size != writeSize)
	{
		return false;
	}

	return true;
}

V3DPipelineColorBlendAttachment ve::InitializeColorBlendAttachment(BLEND_MODE mode, V3DFlags writeMask)
{
	static constexpr V3DPipelineColorBlendAttachment table[] =
	{
		{ false, V3D_BLEND_FACTOR_ONE, V3D_BLEND_FACTOR_ZERO , V3D_BLEND_OP_ADD, V3D_BLEND_FACTOR_ONE, V3D_BLEND_FACTOR_ZERO, V3D_BLEND_OP_ADD, V3D_COLOR_COMPONENT_ALL },
		{ true, V3D_BLEND_FACTOR_SRC_ALPHA, V3D_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA , V3D_BLEND_OP_ADD, V3D_BLEND_FACTOR_ONE, V3D_BLEND_FACTOR_ZERO, V3D_BLEND_OP_ADD, V3D_COLOR_COMPONENT_ALL },
		{ true, V3D_BLEND_FACTOR_SRC_ALPHA, V3D_BLEND_FACTOR_ONE , V3D_BLEND_OP_ADD, V3D_BLEND_FACTOR_SRC_ALPHA, V3D_BLEND_FACTOR_ONE, V3D_BLEND_OP_ADD, V3D_COLOR_COMPONENT_ALL },
		{ true, V3D_BLEND_FACTOR_SRC_ALPHA, V3D_BLEND_FACTOR_ONE , V3D_BLEND_OP_REVERSE_SUBTRACT, V3D_BLEND_FACTOR_SRC_ALPHA, V3D_BLEND_FACTOR_ONE, V3D_BLEND_OP_REVERSE_SUBTRACT, V3D_COLOR_COMPONENT_ALL },
		{ true, V3D_BLEND_FACTOR_ZERO, V3D_BLEND_FACTOR_SRC_COLOR , V3D_BLEND_OP_ADD, V3D_BLEND_FACTOR_ZERO, V3D_BLEND_FACTOR_SRC_ALPHA, V3D_BLEND_OP_ADD, V3D_COLOR_COMPONENT_ALL },
		{ true, V3D_BLEND_FACTOR_ONE_MINUS_DST_COLOR, V3D_BLEND_FACTOR_ONE , V3D_BLEND_OP_ADD, V3D_BLEND_FACTOR_ONE_MINUS_DST_ALPHA, V3D_BLEND_FACTOR_ONE, V3D_BLEND_OP_ADD, V3D_COLOR_COMPONENT_ALL },
	};

	V3DPipelineColorBlendAttachment ret = table[mode];
	ret.writeMask = writeMask;

	return ret;
}

namespace ve {

	class Initializer
	{
	public:
		//! @brief コンストラクタ
		Initializer(void)
		{
		}

		//! @brief デストラクタ
		~Initializer(void)
		{
		}
	};
}

#pragma warning( disable: 4074 )

#if defined(__cplusplus) && defined(__GNUC__)
ve::Initializer init __attribute__((init_priority(111)));
#elif defined (_MSC_VER)
#pragma init_seg(compiler)
ve::Initializer init;
#else
#error not supported.
#endif

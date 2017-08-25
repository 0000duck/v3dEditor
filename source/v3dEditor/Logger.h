#pragma once

namespace ve {

	class Logger
	{
	public:
		static auto constexpr MAX_MESSAGE_SIZE = 4096;

		enum TYPE
		{
			TYPE_INFO = 0,
			TYPE_WARNING = 1,
			TYPE_ERROR = 2,
			TYPE_DEBUG = 3,
		};

		struct Item
		{
			Logger::TYPE type;
			StringA message;
		};

		static LoggerPtr Create();
		static LoggerPtr Create(LoggerPtr logger);

		Logger();
		Logger(LoggerPtr logger);
		~Logger();

		void ClearItems();

		bool BeginItem(size_t& itemCount);
		void EndItem();
		const Logger::Item& GetItem(size_t itemIndex) const;

		void PrintA(Logger::TYPE type, const char* pFormat, ...);
		void PrintW(Logger::TYPE type, const wchar_t* pFormat, ...);

		void PushIndent();
		void PopIndent();

		VE_DECLARE_ALLOCATOR

	private:
		static constexpr int32_t TabSize = 2;

		Mutex m_Mutex;

		LoggerPtr m_Logger;
		int32_t m_Indent;
		collection::Vector<Logger::Item> m_Items;
	};

}
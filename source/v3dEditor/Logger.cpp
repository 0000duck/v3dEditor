#include "Logger.h"

namespace ve {

	LoggerPtr Logger::Create()
	{
		return std::make_shared<Logger>();
	}

	LoggerPtr Logger::Create(LoggerPtr logger)
	{
		return std::make_shared<Logger>(logger);
	}

	Logger::Logger() :
		m_Indent(0)
	{
	}

	Logger::Logger(LoggerPtr logger) :
		m_Logger(logger),
		m_Indent(0)
	{
		VE_ASSERT(this != logger.get());
	}

	Logger::~Logger()
	{
	}

	void Logger::ClearItems()
	{
		LockGuard<Mutex> lock(m_Mutex);
		m_Items.clear();

		if (m_Logger != nullptr)
		{
			LockGuard<Mutex> lock(m_Logger->m_Mutex);
			m_Logger->m_Items.clear();
		}
	}

	bool Logger::BeginItem(size_t& itemCount)
	{
		m_Mutex.lock();
		itemCount = m_Items.size();
		return (itemCount > 0);
	}

	void Logger::EndItem()
	{
		m_Mutex.unlock();
	}

	const Logger::Item& Logger::GetItem(size_t itemIndex) const
	{
		return m_Items[itemIndex];
	}

	void Logger::PrintA(Logger::TYPE type, const char* pFormat, ...)
	{
		LockGuard<Mutex> lock(m_Mutex);

		char temp[Logger::MAX_MESSAGE_SIZE] = "";
		va_list args;

		va_start(args, pFormat);
		_vsnprintf_s(temp, sizeof(temp), _TRUNCATE, pFormat, args);
		va_end(args);

		Logger::Item item;
		item.type = type;
		item.message = temp;

		if (m_Indent > 0)
		{
			item.message.insert(0, Logger::TabSize * m_Indent, L' ');
		}

#ifdef _DEBUG
		OutputDebugStringA(item.message.c_str());
		OutputDebugStringA("\n");
#endif //_DEBUG

		m_Items.push_back(item);

		if (m_Logger != nullptr)
		{
			LockGuard<Mutex> lock(m_Logger->m_Mutex);
			m_Logger->m_Items.push_back(item);
		}
	}

	void Logger::PrintW(Logger::TYPE type, const wchar_t* pFormat, ...)
	{
		LockGuard<Mutex> lock(m_Mutex);

		wchar_t temp[Logger::MAX_MESSAGE_SIZE] = L"";
		va_list args;

		va_start(args, pFormat);
		_vsnwprintf_s(temp, sizeof(temp) >> 1, _TRUNCATE, pFormat, args);
		va_end(args);

		Logger::Item item;
		item.type = type;

		ToMultibyteString(temp, item.message);

		if (m_Indent > 0)
		{
			item.message.insert(0, Logger::TabSize * m_Indent, L' ');
		}

#ifdef _DEBUG
		OutputDebugStringA(item.message.c_str());
		OutputDebugStringA("\n");
#endif //_DEBUG

		m_Items.push_back(item);

		if (m_Logger != nullptr)
		{
			LockGuard<Mutex> lock(m_Logger->m_Mutex);
			m_Logger->m_Items.push_back(item);
		}
	}

	void Logger::PushIndent()
	{
		LockGuard<Mutex> lock(m_Mutex);
		m_Indent++;
	}

	void Logger::PopIndent()
	{
		LockGuard<Mutex> lock(m_Mutex);
		m_Indent = std::max(0, m_Indent - 1);
	}

}
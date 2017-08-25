#include "BackgroundQueue.h"
#include "SkeletalModel.h"
#include <chrono>

namespace ve {

	/*****************************/
	/* class BackgroundJobHandle */
	/*****************************/

	BackgroundJobHandle::BackgroundJobHandle() :
		m_State(BACKGROUND_JOB_PENDING)
	{
	}

	BackgroundJobHandle::~BackgroundJobHandle()
	{
	}

	BACKGROUND_JOB_STATE BackgroundJobHandle::GetState()
	{
		return m_State;
	}

	bool BackgroundJobHandle::IsFinished()
	{
		return m_State >= BACKGROUND_JOB_COMPLETED;
	}

	/***********************/
	/* private - class Job */
	/***********************/

	BackgroundQueue::Job::Job(BackgroundJobHandlePtr handle) :
		m_Handle(handle)
	{
	}

	BackgroundQueue::Job::~Job()
	{
	}

	void BackgroundQueue::Job::Execute()
	{
		BACKGROUND_JOB_STATE currentState = BACKGROUND_JOB_PENDING;

		if (m_Handle->m_State.compare_exchange_weak(currentState, BACKGROUND_JOB_RUNNING) == true)
		{
			currentState = BACKGROUND_JOB_RUNNING;

			BACKGROUND_JOB_STATE nextState = OnExecute();
			if (m_Handle->m_State.compare_exchange_weak(currentState, nextState) == false)
			{
				m_Handle->m_State.compare_exchange_strong(currentState, BACKGROUND_JOB_INTERNAL_ERROR);
			}
		}
		else
		{
			m_Handle->m_State.compare_exchange_strong(currentState, BACKGROUND_JOB_INTERNAL_ERROR);
		}
	}

	/*****************************/
	/* private - class ImportJob */
	/*****************************/

	class BackgroundQueue::ImportJob final : public BackgroundQueue::Job
	{
	public:
		ImportJob(
			BackgroundJobHandlePtr handle,
			LoggerPtr logger,
			DeviceContextPtr deviceContext,
			const wchar_t* pSourceFilePath, const ModelSourceConfig& sourceConfig, ModelSourcePtr source,
			const ModelRendererConfig& rendererConfig, SkeletalModelPtr renderer) : Job(handle),
			m_Logger(logger),
			m_DeviceContext(deviceContext),
			m_SourceFilePath(pSourceFilePath),
			m_SourceConfig(sourceConfig),
			m_Source(source),
			m_RendererConfig(rendererConfig),
			m_Renderer(renderer)
		{
		}

		~ImportJob()
		{
		}

		BACKGROUND_JOB_STATE OnExecute() override
		{
			if (m_Source->Load(m_Logger, m_DeviceContext, m_SourceFilePath.c_str(), m_SourceConfig) == false)
			{
				return BACKGROUND_JOB_ERROR;
			}

			if (m_Renderer->Load(m_Logger, m_Source, m_RendererConfig) == false)
			{
				return BACKGROUND_JOB_ERROR;
			}

			SkeletalModel::Finish(m_Logger, m_Renderer);

			return BACKGROUND_JOB_COMPLETED;
		}

	private:
		LoggerPtr m_Logger;
		DeviceContextPtr m_DeviceContext;

		StringW m_SourceFilePath;
		ModelSourceConfig m_SourceConfig;
		ModelSourcePtr m_Source;

		ModelRendererConfig m_RendererConfig;
		SkeletalModelPtr m_Renderer;
	};

	/****************************/
	/* public - BackgroundQueue */
	/****************************/

	BackgroundQueuePtr BackgroundQueue::Create()
	{
		BackgroundQueuePtr backgroundQueue = std::make_shared<BackgroundQueue>();

		if (backgroundQueue->Initialize() == false)
		{
			return nullptr;
		}

		return std::move(backgroundQueue);
	}

	BackgroundQueue::BackgroundQueue()
	{
	}

	BackgroundQueue::~BackgroundQueue()
	{
	}

	void BackgroundQueue::Terminate()
	{
		// ----------------------------------------------------------------------------------------------------
		// スレッドを終了を通知
		// ----------------------------------------------------------------------------------------------------

		{
			std::unique_lock<std::mutex> lock(m_Mutex);
			m_Jobs.push_back(nullptr);
			m_Condition.notify_one();
		}

		// ----------------------------------------------------------------------------------------------------
		// スレッドを終了を待機
		// ----------------------------------------------------------------------------------------------------

		m_Thread.join();
	}

	BackgroundJobHandlePtr BackgroundQueue::AddImportJob(
		LoggerPtr logger,
		DeviceContextPtr deviceContext,
		const wchar_t* pSourceFilePath, const ModelSourceConfig& sourceConfig, ModelSourcePtr source,
		const ModelRendererConfig& rendererConfig, SkeletalModelPtr renderer)
	{
		BackgroundJobHandlePtr handle = std::make_shared<BackgroundJobHandle>();

		std::shared_ptr<BackgroundQueue::ImportJob> job = std::make_shared<BackgroundQueue::ImportJob>(
			handle,
			logger,
			deviceContext,
			pSourceFilePath, sourceConfig, source,
			rendererConfig, renderer);

		std::unique_lock<std::mutex> lock(m_Mutex);
		m_Jobs.push_back(job);
		m_Condition.notify_one();

		return handle;
	}

	/*****************************/
	/* private - BackgroundQueue */
	/*****************************/

	bool BackgroundQueue::Initialize()
	{
		m_Thread = std::thread(BackgroundQueue::ThreadEntry, this);

		return true;
	}

	void BackgroundQueue::Process()
	{
		bool continueLoop = true;

		do
		{
			std::shared_ptr<BackgroundQueue::Job> job;

			{
				std::unique_lock<std::mutex> lock(m_Mutex);
				m_Condition.wait(lock, [this]() {return m_Jobs.empty() == false; });

				job = m_Jobs.front();
				m_Jobs.pop_front();
			}

			if (job != nullptr)
			{
				job->Execute();
			}
			else
			{
				continueLoop = false;
			}

		} while (continueLoop == true);
	}

	void BackgroundQueue::ThreadEntry(void* pData)
	{
		static_cast<BackgroundQueue*>(pData)->Process();
	}

}
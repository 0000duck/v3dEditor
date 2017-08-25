#pragma once

#include <condition_variable>
#include <thread>
#include <atomic>
#include "IModelSource.h"

namespace ve {

	// ----------------------------------------------------------------------------------------------------

	enum BACKGROUND_JOB_STATE
	{
		BACKGROUND_JOB_PENDING = 0,
		BACKGROUND_JOB_RUNNING = 1,
		BACKGROUND_JOB_COMPLETED = 2,

		BACKGROUND_JOB_ERROR = 3,
		BACKGROUND_JOB_INTERNAL_ERROR = 4,
	};

	// ----------------------------------------------------------------------------------------------------

	class BackgroundJobHandle final
	{
	public:
		BackgroundJobHandle();
		~BackgroundJobHandle();

		BACKGROUND_JOB_STATE GetState();
		bool IsFinished();

	private:
		std::atomic<BACKGROUND_JOB_STATE> m_State;

		friend class BackgroundQueue;
	};

	// ----------------------------------------------------------------------------------------------------

	class BackgroundQueue final
	{
	public:
		static BackgroundQueuePtr Create();

		BackgroundQueue();
		~BackgroundQueue();

		void Terminate();

		BackgroundJobHandlePtr AddImportJob(
			LoggerPtr logger,
			DeviceContextPtr deviceContext,
			const wchar_t* pSourceFilePath, const ModelSourceConfig& sourceConfig, ModelSourcePtr source,
			const ModelRendererConfig& rendererConfig, SkeletalModelPtr renderer);

		VE_DECLARE_ALLOCATOR

	private:
		class Job
		{
		public:
			Job(BackgroundJobHandlePtr handle);
			virtual ~Job();

			void Execute();

		protected:
			BackgroundJobHandlePtr m_Handle;

			virtual BACKGROUND_JOB_STATE OnExecute() = 0;
		};

		class ImportJob;

		std::thread m_Thread;
		std::mutex m_Mutex;
		std::condition_variable m_Condition;
		collection::List<std::shared_ptr<Job>> m_Jobs;

		bool Initialize();
		void Process();

		static void ThreadEntry(void* pData);
	};

}
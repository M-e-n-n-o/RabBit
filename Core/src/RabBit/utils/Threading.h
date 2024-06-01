#pragma once

#include "RabBitCommon.h"

namespace RB
{
	enum class ThreadPriority
	{
		Low		= 0,
		Medium	= 1,
		High	= 2,
		Highest	= 3,

		Default	= Medium
	};

	using JobTypeID		= uint32_t;
	using JobID			= uint64_t;
	using JobData		= void;
	using JobFunction	= std::function<void(JobData*)>;

	// This class itself is NOT threadsafe, should be owned/used by 1 thread at a time!
	class WorkerThread
	{
	public:
		WorkerThread(const wchar_t* name, const ThreadPriority& priority = ThreadPriority::Default);

		~WorkerThread();

		// If overwritable is true, only 1 of this type of job can be scheduled at a time.
		// So, if a job is scheduled that is already in the queue, the old job will be overwritten.
		JobTypeID	AddJobType(JobFunction* function, bool overwritable = false);

		// Job data will be free'd after finished/being overwritten
		JobID		ScheduleJob(JobTypeID type_id, JobData* data);

		void		PrioritizeJob(JobID job_id);

		bool		IsFinished(JobID job_id);
		void		Sync(JobID job_id);
		void		SyncAll();

		bool		IsStalled(uint32_t stall_threshold_ms, JobID& out_id);

		void		Cancel(JobID job_id);
		void		CancelAll();

	private:
		struct Job
		{
			JobID				id;
			JobFunction*		function;
			JobData*			data;
		};

		List<Job>::iterator FindJobBy(JobID id);

		enum class ThreadState
		{
			Idle,
			Waking,
			Running,
			Terminating,
			Terminated
		};

		struct SharedContext
		{
			const wchar_t*		name;

			ThreadState			state;
			CONDITION_VARIABLE	kickCV;
			CRITICAL_SECTION	kickCS;
			CONDITION_VARIABLE	syncCV;
			CRITICAL_SECTION	syncCS;
			CONDITION_VARIABLE	completedCV;
			CRITICAL_SECTION	completedCS;

			uint64_t			counterStart;

			JobID				currentJob;
			List<Job>			pendingJobs;
			uint32_t			highPriorityInsertIndex;
			uint64_t			startedJobsCount;
			bool				syncCompleted;
		};

		struct JobType
		{
			JobFunction*		function;
			bool				overwritable;
		};

		HANDLE					m_ThreadHandle;
		SharedContext*			m_SharedContext;
		List<JobType>			m_JobTypes;
		double					m_PerformanceFreqMs;

		friend DWORD WINAPI WorkerThreadLoop(PVOID param);
	};
}
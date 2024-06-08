#include "RabBitCommon.h"
#include "Threading.h"

namespace RB
{
	DWORD WINAPI WorkerThreadLoop(PVOID param);

	WorkerThread::WorkerThread(const wchar_t* name, const ThreadPriority& priority)
	{
		LARGE_INTEGER li;
		if (!QueryPerformanceFrequency(&li))
		{
			RB_LOG_ERROR(LOGTAG_MAIN, "Could not retrieve value from QueryPerformanceFrequency");
		}

		m_PerformanceFreqMs = double(li.QuadPart) / 1000.0;

		m_SharedContext = new SharedContext();
		m_SharedContext->name					 = name;
		m_SharedContext->state					 = ThreadState::Idle;
		m_SharedContext->currentJob				 = 0;
		m_SharedContext->pendingJobs			 = {};
		m_SharedContext->highPriorityInsertIndex = 0;
		m_SharedContext->startedJobsCount		 = 0;
		m_SharedContext->completedJobsCount		 = 0;
		m_SharedContext->counterStart			 = 0;

		InitializeConditionVariable(&m_SharedContext->kickCV);
		InitializeConditionVariable(&m_SharedContext->syncCV);
		InitializeConditionVariable(&m_SharedContext->completedCV);
		InitializeCriticalSection(&m_SharedContext->kickCS);
		InitializeCriticalSection(&m_SharedContext->syncCS);
		InitializeCriticalSection(&m_SharedContext->completedCS);

		DWORD id;
		m_ThreadHandle = CreateThread(NULL, 0, WorkerThreadLoop, (PVOID) m_SharedContext, 0, &id);
		RB_ASSERT_FATAL_RELEASE(LOGTAG_MAIN, m_ThreadHandle != 0, "Failed to create worker thread thread");

		SetThreadDescription(m_ThreadHandle, name);

		int job_prio = 0;
		switch (priority)
		{
		case ThreadPriority::Low:	  job_prio = THREAD_PRIORITY_BELOW_NORMAL;	break;
		case ThreadPriority::Medium:  job_prio = THREAD_PRIORITY_NORMAL;		break;
		case ThreadPriority::High:	  job_prio = THREAD_PRIORITY_ABOVE_NORMAL;  break;
		case ThreadPriority::Highest: job_prio = THREAD_PRIORITY_HIGHEST;		break;
		default:
			RB_LOG_ERROR(LOGTAG_MAIN, "Did not implement this thread priority yet");
			break;
		}

		SetThreadPriority(m_ThreadHandle, job_prio);
	}

	WorkerThread::~WorkerThread()
	{
		SyncAll();

		EnterCriticalSection(&m_SharedContext->kickCS);
		m_SharedContext->state = ThreadState::Terminating;
		LeaveCriticalSection(&m_SharedContext->kickCS);
		WakeConditionVariable(&m_SharedContext->kickCV);

		WaitForMultipleObjects(1, &m_ThreadHandle, TRUE, INFINITE);
		CloseHandle(m_ThreadHandle);

		delete m_SharedContext;
	}

	JobTypeID WorkerThread::AddJobType(JobFunction function, bool overwritable)
	{
		JobType type = {};
		type.function		= function;
		type.overwritable	= overwritable;

		m_JobTypes.push_back(type);

		return m_JobTypes.size() - 1;
	}

	JobID WorkerThread::ScheduleJob(JobTypeID type_id, JobData* data)
	{
		if (type_id < 0 || type_id > m_JobTypes.size() - 1)
		{
			RB_LOG_ERROR(LOGTAG_MAIN, "Could not schedule job, JobType does not exist");
			return 0;
		}

		JobType& type = m_JobTypes[type_id];

		EnterCriticalSection(&m_SharedContext->kickCS);
		
		m_SharedContext->state = ThreadState::Waking;

		Job job = {};
		job.id		 = m_SharedContext->startedJobsCount + m_SharedContext->pendingJobs.size();
		job.function = &type.function;
		job.data	 = data;

		if (type.overwritable)
		{
			auto itr = std::find_if(m_SharedContext->pendingJobs.begin(), m_SharedContext->pendingJobs.end(), [job](Job& other) -> bool
				{
					return job.function == other.function;
				});

			if (itr == m_SharedContext->pendingJobs.end())
			{
				// Insert new job
				m_SharedContext->pendingJobs.push_back(job);
			}
			else
			{
				// Job data is overwritten
				SAFE_DELETE(itr->data);
				itr->data = data;
				job.id = itr->id;
			}
		}
		else
		{
			// Insert new job
			m_SharedContext->pendingJobs.push_back(job);
		}

		LeaveCriticalSection(&m_SharedContext->kickCS);
		WakeConditionVariable(&m_SharedContext->kickCV);

		return job.id;
	}

	void WorkerThread::PrioritizeJob(JobID job_id)
	{
		EnterCriticalSection(&m_SharedContext->kickCS);

		auto itr = FindJobBy(job_id);

		if (itr == m_SharedContext->pendingJobs.end())
		{
			LeaveCriticalSection(&m_SharedContext->kickCS);
			return;
		}

		auto high_prio_itr = m_SharedContext->pendingJobs.begin() + m_SharedContext->highPriorityInsertIndex;

		if (itr > high_prio_itr)
		{
			// Move the job to last place of the high priority queue
			std::rotate(high_prio_itr, itr, itr + 1);

			m_SharedContext->highPriorityInsertIndex++;
		}

		LeaveCriticalSection(&m_SharedContext->kickCS);
	}

	bool WorkerThread::IsFinished(JobID job_id)
	{
		EnterCriticalSection(&m_SharedContext->kickCS);

		auto itr = FindJobBy(job_id);

		if (itr == m_SharedContext->pendingJobs.end() && m_SharedContext->currentJob != job_id)
		{
			LeaveCriticalSection(&m_SharedContext->kickCS);
			return true;
		}

		LeaveCriticalSection(&m_SharedContext->kickCS);

		return false;
	}

	void WorkerThread::Sync(JobID job_id)
	{
		EnterCriticalSection(&m_SharedContext->kickCS);

		uint64_t wait_for = m_SharedContext->startedJobsCount;

		if (job_id != m_SharedContext->currentJob)
		{
			auto itr = FindJobBy(job_id);

			if (itr == m_SharedContext->pendingJobs.end())
			{
				// Job not found
				LeaveCriticalSection(&m_SharedContext->kickCS);
				return;
			}

			wait_for += std::distance(m_SharedContext->pendingJobs.begin(), itr) + 1;
		}

		LeaveCriticalSection(&m_SharedContext->kickCS);

		// Wait until the task has been completed
		{
			EnterCriticalSection(&m_SharedContext->completedCS);

			while (m_SharedContext->completedJobsCount < wait_for)
			{
				SleepConditionVariableCS(&m_SharedContext->completedCV, &m_SharedContext->completedCS, INFINITE);
			}

			LeaveCriticalSection(&m_SharedContext->completedCS);
		}
	}

	void WorkerThread::SyncAll()
	{
		// Wait until thread completely idle
		EnterCriticalSection(&m_SharedContext->syncCS);

		while (m_SharedContext->state != ThreadState::Idle)
		{
			SleepConditionVariableCS(&m_SharedContext->syncCV, &m_SharedContext->syncCS, INFINITE);
		}

		LeaveCriticalSection(&m_SharedContext->syncCS);
	}

	bool WorkerThread::IsStalling(uint32_t stall_threshold_ms, JobID& out_id)
	{
		EnterCriticalSection(&m_SharedContext->kickCS);
		ThreadState state		  = m_SharedContext->state;
		uint64_t	counter_start = m_SharedContext->counterStart;
		JobID		current_job	  = m_SharedContext->currentJob;
		LeaveCriticalSection(&m_SharedContext->kickCS);

		if (state != ThreadState::Idle)
		{
			LARGE_INTEGER li;
			QueryPerformanceCounter(&li);

			if ((double(li.QuadPart - counter_start) / m_PerformanceFreqMs) > stall_threshold_ms)
			{
				out_id = current_job;
				return true;
			}
		}

		return false;
	}

	void WorkerThread::Cancel(JobID job_id)
	{
		EnterCriticalSection(&m_SharedContext->kickCS);

		auto itr = FindJobBy(job_id);

		if (itr == m_SharedContext->pendingJobs.end())
		{
			LeaveCriticalSection(&m_SharedContext->kickCS);
			return;
		}

		SAFE_DELETE(itr->data);
		m_SharedContext->pendingJobs.erase(itr);

		LeaveCriticalSection(&m_SharedContext->kickCS);
	}

	void WorkerThread::CancelAll()
	{
		EnterCriticalSection(&m_SharedContext->kickCS);

		for (int i = 0; i < m_SharedContext->pendingJobs.size(); ++i)
		{
			SAFE_DELETE(m_SharedContext->pendingJobs[i].data);
		}
		m_SharedContext->pendingJobs.clear();

		LeaveCriticalSection(&m_SharedContext->kickCS);
	}

	List<WorkerThread::Job>::iterator WorkerThread::FindJobBy(JobID id)
	{
		return std::find_if(m_SharedContext->pendingJobs.begin(), m_SharedContext->pendingJobs.end(), [id](Job& other) -> bool
			{
				return id == other.id;
			});
	}

	DWORD WINAPI WorkerThreadLoop(PVOID param)
	{
		WorkerThread::SharedContext* context = (WorkerThread::SharedContext*) param;

		RB_LOG(LOGTAG_MAIN, "Started worker thread: %ws", context->name);

		while (true)
		{
			WorkerThread::Job current_job;

			// Wait until a new task is available
			{
				EnterCriticalSection(&context->kickCS);

				// Reset the timer
				context->counterStart = 0;

				context->currentJob = 0;

				// Only start waiting if there are no more jobs pending
				if (context->pendingJobs.empty())
				{
					// Wait until kick
					do
					{
						// Notify that we are starting to idle
						{
							EnterCriticalSection(&context->syncCS);
							context->state = WorkerThread::ThreadState::Idle;
							LeaveCriticalSection(&context->syncCS);
							WakeConditionVariable(&context->syncCV);
						}

						// Sleep
						SleepConditionVariableCS(&context->kickCV, &context->kickCS, INFINITE);

					} while (context->state == WorkerThread::ThreadState::Idle);
				}

				if (context->state == WorkerThread::ThreadState::Terminating)
				{
					LeaveCriticalSection(&context->kickCS);
					break;
				}

				// Copy over the job
				current_job = {};
				current_job.id		 = context->pendingJobs[0].id;
				current_job.function = context->pendingJobs[0].function;
				current_job.data	 = context->pendingJobs[0].data;

				context->pendingJobs.erase(context->pendingJobs.begin());

				context->startedJobsCount++;
				context->highPriorityInsertIndex = Math::Max((int)context->highPriorityInsertIndex - 1, 0);

				context->currentJob = current_job.id;

				// Refresh our state
				context->state = WorkerThread::ThreadState::Running;

				// Start timer
				LARGE_INTEGER li;
				QueryPerformanceCounter(&li);
				context->counterStart = li.QuadPart;

				LeaveCriticalSection(&context->kickCS);
			}

			// Do the job
			{
				(*current_job.function)(current_job.data);
				SAFE_DELETE(current_job.data);
			}

			// Notify that we are done with a job
			{
				EnterCriticalSection(&context->completedCS);
				context->completedJobsCount++;
				LeaveCriticalSection(&context->completedCS);
				WakeConditionVariable(&context->completedCV);
			}
		}

		RB_LOG(LOGTAG_MAIN, "Terminated worker thread: %s", context->name);

		context->state = WorkerThread::ThreadState::Terminated;

		return 0;
	}
}
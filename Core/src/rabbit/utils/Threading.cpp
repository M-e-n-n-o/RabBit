#include "RabBitCommon.h"
#include "Threading.h"

#if RB_PLATFORM_WINDOWS
#include <windows.h>
#include <processthreadsapi.h>
#elif RB_PLATFORM_LINUX_ES
#include <pthread.h>
#endif

namespace RB
{
    // ---------------------------------------------------------------------------
    //								WorkerThread
    // ---------------------------------------------------------------------------

    void WorkerThreadLoop(WorkerThread::SharedContext* context);

    WorkerThread::WorkerThread(const char* name, const ThreadPriority& priority)
    {
        m_SharedContext = new SharedContext();
        m_SharedContext->name                       = name;
        m_SharedContext->state                      = ThreadState::Idle;
        m_SharedContext->currentJob                 = UINT64_MAX;
        m_SharedContext->pendingJobs                = {};
        m_SharedContext->highPriorityInsertIndex    = 0;
        m_SharedContext->startedJobsCount           = 0;
        m_SharedContext->completedJobsCount         = 0;
        m_SharedContext->counterStart               = 0;

        m_ThreadHandle = std::thread(WorkerThreadLoop, m_SharedContext);

        // Set the name of the thread + thread priority
#if RB_PLATFORM_WINDOWS
        wchar_t* wchar_name = new wchar_t[strlen(name) + 1];
        CharToWchar(name, wchar_name);
        SetThreadDescription(static_cast<HANDLE>(m_ThreadHandle.native_handle()), wchar_name);
        delete[] wchar_name;

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

        SetThreadPriority(static_cast<HANDLE>(m_ThreadHandle.native_handle()), job_prio);
#elif RB_PLATFORM_LINUX_ES
        pthread_setname_np(m_ThreadHandle.native_handle(), name);

        sched_param param;
        int policy;
        pthread_getschedparam(m_ThreadHandle.native_handle(), &policy, &param);

        switch (priority)
        {
        case ThreadPriority::Low:
            param.sched_priority = 1;
            break;
        case ThreadPriority::Medium:
            param.sched_priority = 50;
            break;
        case ThreadPriority::High:
            param.sched_priority = 75;
            break;
        case ThreadPriority::Highest:
            param.sched_priority = 99;
            break;
        default:
            RB_LOG_ERROR(LOGTAG_MAIN, "Did not implement this thread priority yet");
            break;
        }

        // Try SCHED_FIFO first (requires root privileges)
        if (pthread_setschedparam(m_ThreadHandle.native_handle(), SCHED_FIFO, &param) != 0)
        {
            // If SCHED_FIFO fails, fall back to SCHED_OTHER
            policy = SCHED_OTHER;
            param.sched_priority = 0;  // SCHED_OTHER only allows 0
            pthread_setschedparam(m_ThreadHandle.native_handle(), policy, &param);

            RB_LOG_WARN(LOGTAG_MAIN, "Failed to set thread prioriy. The app is likely missing root privileges");
        }
#endif
    }

    WorkerThread::~WorkerThread()
    {
        SyncAll();

        m_SharedContext->kickMutex.lock();
        m_SharedContext->state = ThreadState::Terminating;
        m_SharedContext->kickMutex.unlock();
        m_SharedContext->kickCV.notify_one();

        m_ThreadHandle.join();

        delete m_SharedContext;
    }

    JobTypeID WorkerThread::AddJobType(JobFunction function, bool overwritable)
    {
        JobType type = {};
        type.function = function;
        type.overwritable = overwritable;

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

        m_SharedContext->kickMutex.lock();

        m_SharedContext->state = ThreadState::Waking;

        Job job = {};
        job.id          = m_SharedContext->startedJobsCount + m_SharedContext->pendingJobs.size();
        job.function    = &type.function;
        job.data        = data;

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

        m_SharedContext->kickMutex.unlock();
        m_SharedContext->kickCV.notify_one();

        return job.id;
    }

    void WorkerThread::PrioritizeJob(JobID job_id)
    {
        RB_MUTEX_AUTO_LOCK(m_SharedContext->kickMutex);

        auto itr = FindJobBy(job_id);

        if (itr == m_SharedContext->pendingJobs.end())
        {
            return;
        }

        auto high_prio_itr = m_SharedContext->pendingJobs.begin() + m_SharedContext->highPriorityInsertIndex;

        if (itr > high_prio_itr)
        {
            // Move the job to last place of the high priority queue
            std::rotate(high_prio_itr, itr, itr + 1);

            m_SharedContext->highPriorityInsertIndex++;
        }
        else if (itr == high_prio_itr)
        {
            // Make sure nothing can get placed before this
            m_SharedContext->highPriorityInsertIndex++;
        }
    }

    bool WorkerThread::IsFinished(JobID job_id)
    {
        RB_MUTEX_AUTO_LOCK(m_SharedContext->kickMutex);

        auto itr = FindJobBy(job_id);

        if (itr == m_SharedContext->pendingJobs.end() && m_SharedContext->currentJob != job_id)
        {
            return true;
        }

        return false;
    }

    void WorkerThread::Sync(JobID job_id)
    {
        m_SharedContext->kickMutex.lock();
        
        uint64_t wait_for = m_SharedContext->startedJobsCount;
        
        if (job_id != m_SharedContext->currentJob)
        {
            auto itr = FindJobBy(job_id);
        
            if (itr == m_SharedContext->pendingJobs.end())
            {
                // Job not found
                m_SharedContext->kickMutex.unlock();
                return;
            }
        
            wait_for += std::distance(m_SharedContext->pendingJobs.begin(), itr) + 1;
        }
        
        m_SharedContext->kickMutex.unlock();
        
        // Wait until the task has been completed
        {
            std::unique_lock<Mutex> lock(m_SharedContext->completedMutex);

            m_SharedContext->completedCV.wait(lock, [&] {
                // TODO This logic will break when syncing a job that has not been prioritized as other jobs can then jump before this one, fix!!!
                return m_SharedContext->completedJobsCount >= wait_for;
            });
        }
    }

    void WorkerThread::SyncAll()
    {
        // Wait until thread completely idle
        std::unique_lock<Mutex> lock(m_SharedContext->syncMutex);

        m_SharedContext->syncCV.wait(lock, [&] {
            return m_SharedContext->state == ThreadState::Idle;
        });
    }

    bool WorkerThread::IsStalling(uint32_t stall_threshold_ms, JobID& out_id)
    {
        m_SharedContext->kickMutex.lock();
        ThreadState state           = m_SharedContext->state;
        double      counter_start   = m_SharedContext->counterStart;
        double      current_time    = m_SharedContext->timer.ElapsedMilliseconds();
        JobID       current_job     = m_SharedContext->currentJob;
        m_SharedContext->kickMutex.unlock();

        if (state != ThreadState::Idle)
        {
            if ((current_time - counter_start) > stall_threshold_ms)
            {
                out_id = current_job;
                return true;
            }
        }

        return false;
    }

    void WorkerThread::Cancel(JobID job_id)
    {
        RB_MUTEX_AUTO_LOCK(m_SharedContext->kickMutex);

        auto itr = FindJobBy(job_id);

        if (itr == m_SharedContext->pendingJobs.end())
        {
            return;
        }

        SAFE_DELETE(itr->data);
        m_SharedContext->pendingJobs.erase(itr);
    }

    void WorkerThread::CancelAll()
    {
        RB_MUTEX_AUTO_LOCK(m_SharedContext->kickMutex);

        for (int i = 0; i < m_SharedContext->pendingJobs.size(); ++i)
        {
            SAFE_DELETE(m_SharedContext->pendingJobs[i].data);
        }
        m_SharedContext->pendingJobs.clear();
    }

    bool WorkerThread::IsCurrentThread()
    {
        return std::this_thread::get_id() == m_ThreadHandle.get_id();
    }

    List<WorkerThread::Job>::iterator WorkerThread::FindJobBy(JobID id)
    {
        return std::find_if(m_SharedContext->pendingJobs.begin(), m_SharedContext->pendingJobs.end(), [id](Job& other) -> bool
            {
                return id == other.id;
            });
    }

    void WorkerThreadLoop(WorkerThread::SharedContext* context)
    {
        RB_LOG(LOGTAG_MAIN, "Started worker thread: %s", context->name);
        
        context->timer.Reset();

        while (true)
        {
            WorkerThread::Job current_job;

            // Wait until a new task is available
            {
                std::unique_lock<Mutex> kick_lock(context->kickMutex);

                context->currentJob = UINT64_MAX;

                // Only start waiting if there are no more jobs pending
                if (context->pendingJobs.empty())
                {
                    // Wait until kick
                    do
                    {
                        // Notify that we are starting to idle
                        {
                            context->syncMutex.lock();
                            context->state = WorkerThread::ThreadState::Idle;
                            context->syncMutex.unlock();
                            context->syncCV.notify_one();
                        }

                        // Sleep
                        context->kickCV.wait(kick_lock);

                    } while (context->state == WorkerThread::ThreadState::Idle);
                }

                if (context->state == WorkerThread::ThreadState::Terminating)
                {
                    break;
                }

                // Copy over the job
                current_job = {};
                current_job.id       = context->pendingJobs[0].id;
                current_job.function = context->pendingJobs[0].function;
                current_job.data     = context->pendingJobs[0].data;

                context->pendingJobs.erase(context->pendingJobs.begin());

                context->startedJobsCount++;
                context->highPriorityInsertIndex = Math::Max((int)context->highPriorityInsertIndex - 1, 0);

                context->currentJob = current_job.id;

                // Refresh our state
                context->state = WorkerThread::ThreadState::Running;

                // Start timer
                context->counterStart = context->timer.ElapsedMilliseconds();
            }

            // Do the job
            {
                (*current_job.function)(current_job.data);
                SAFE_DELETE(current_job.data);
            }

            // Notify that we are done with a job
            {
                context->completedMutex.lock();
                context->completedJobsCount++;
                context->completedMutex.unlock();
                context->completedCV.notify_all();
            }
        }

        RB_LOG(LOGTAG_MAIN, "Terminated worker thread: %s", context->name);

        context->state = WorkerThread::ThreadState::Terminated;
    }
}
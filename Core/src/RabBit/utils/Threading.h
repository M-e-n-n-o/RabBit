#pragma once

#include "RabBitCommon.h"

namespace RB
{
    // ---------------------------------------------------------------------------
    //								WorkerThread
    // ---------------------------------------------------------------------------

    enum class ThreadPriority
    {
        Low         = 0,
        Medium      = 1,
        High        = 2,
        Highest     = 3,

        Default     = Medium
    };

    // Make sure to do all your deletes and free's in the destructor!
    struct JobData
    {
        virtual ~JobData() = default;
    };

    using JobTypeID     = uint32_t;
    using JobID         = uint64_t;
    using JobFunction   = std::function<void(JobData*)>;

    // This class itself is NOT threadsafe, should be owned/used by 1 thread at a time!
    class WorkerThread
    {
    public:
        WorkerThread(const wchar_t* name, const ThreadPriority& priority = ThreadPriority::Default);

        ~WorkerThread();

        // If overwritable is true, only 1 of this type of job can be scheduled at a time.
        // So, if a job is scheduled that is already in the queue, the old job will be overwritten.
        JobTypeID	AddJobType(JobFunction function, bool overwritable = false);

        // The JobData is deleted when the task is completed or has been overwritten (allocate the data with new!)
        JobID		ScheduleJob(JobTypeID type_id, JobData* data);

        void		PrioritizeJob(JobID job_id);

        bool		IsFinished(JobID job_id);
        bool		IsStalling(uint32_t stall_threshold_ms, JobID& out_id);

        void		Sync(JobID job_id);
        void		SyncAll();

        void		Cancel(JobID job_id);
        void		CancelAll();

        bool		IsCurrentThread();

    private:
        struct Job
        {
            JobID        id;
            JobFunction* function;
            JobData*     data;
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
            const wchar_t*      name;

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
            uint64_t			completedJobsCount;
        };

        struct JobType
        {
            JobFunction			function;
            bool				overwritable;
        };

        HANDLE					m_ThreadHandle;
        SharedContext*          m_SharedContext;
        List<JobType>			m_JobTypes;
        double					m_PerformanceFreqMs;

        friend DWORD WINAPI WorkerThreadLoop(PVOID param);
    };

    // ---------------------------------------------------------------------------
    //							ThreadedVariable
    // ---------------------------------------------------------------------------

    template<typename T>
    class ThreadedVariable
    {
    public:
        ThreadedVariable(const T& value);
        ~ThreadedVariable();

        void SetValue(const T& value);
        T	 GetValue();

        void WaitUntilConditionMet(std::function<bool(const T&)> condition);

    private:
        T					m_Variable;
        CRITICAL_SECTION	m_CS;
        CONDITION_VARIABLE	m_CV;
    };

    template<typename T>
    inline ThreadedVariable<T>::ThreadedVariable(const T& value)
    {
        m_Variable = value;
        InitializeCriticalSection(&m_CS);
        InitializeConditionVariable(&m_CV);
    }

    template<typename T>
    inline ThreadedVariable<T>::~ThreadedVariable()
    {
        DeleteCriticalSection(&m_CS);
    }

    template<typename T>
    inline void ThreadedVariable<T>::SetValue(const T& value)
    {
        EnterCriticalSection(&m_CS);
        m_Variable = value;
        LeaveCriticalSection(&m_CS);
        WakeAllConditionVariable(&m_CV);
    }

    template<typename T>
    inline T ThreadedVariable<T>::GetValue()
    {
        EnterCriticalSection(&m_CS);
        const T& value = m_Variable;
        LeaveCriticalSection(&m_CS);

        return value;
    }

    template<typename T>
    inline void ThreadedVariable<T>::WaitUntilConditionMet(std::function<bool(const T&)> condition)
    {
        EnterCriticalSection(&m_CS);

        while (!condition(m_Variable))
        {
            SleepConditionVariableCS(&m_CV, &m_CS, INFINITE);
        }

        LeaveCriticalSection(&m_CS);
    }
}
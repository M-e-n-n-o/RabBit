#pragma once

#include "RabBitCommon.h"
#include "Timer.h"

#include <thread>
#include <mutex>
#include <condition_variable>

namespace RB
{
    // ---------------------------------------------------------------------------
    //								    Mutex
    // ---------------------------------------------------------------------------

    // TODO In the future make a wrapper class for this and use SRWLock on Windows to improve performance
    using Mutex             = std::recursive_mutex;
    using ConditionVariable = std::condition_variable_any;

    #define RB_MUTEX_AUTO_LOCK(m) std::lock_guard<Mutex> auto_locking_and_unlocking_mutex(m)

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
        WorkerThread(const char* name, const ThreadPriority& priority = ThreadPriority::Default);

        ~WorkerThread();

        // If overwritable is true, only 1 of this type of job can be scheduled at a time.
        // So, if a job is scheduled that is already in the queue, the old job will be overwritten.
        JobTypeID   AddJobType(JobFunction function, bool overwritable = false);

        // The JobData is deleted when the task is completed or has been overwritten (allocate the data with new!)
        JobID       ScheduleJob(JobTypeID type_id, JobData* data);

        void        PrioritizeJob(JobID job_id);

        bool        IsFinished(JobID job_id);
        bool        IsStalling(uint32_t stall_threshold_ms, JobID& out_id);

        void        Sync(JobID job_id);
        void        SyncAll();

        void        Cancel(JobID job_id);
        void        CancelAll();

        bool        IsCurrentThread();

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
            const char*         name;

            ThreadState         state;
            ConditionVariable   kickCV;
            Mutex               kickMutex;
            ConditionVariable   syncCV;
            Mutex               syncMutex;
            ConditionVariable   completedCV;
            Mutex	            completedMutex;

            Timer               timer;
            double              counterStart;

            JobID               currentJob;
            List<Job>           pendingJobs;
            uint32_t            highPriorityInsertIndex;
            uint64_t            startedJobsCount;
            uint64_t            completedJobsCount;
        };

        struct JobType
        {
            JobFunction         function;
            bool                overwritable;
        };

        std::thread             m_ThreadHandle;            
        SharedContext*          m_SharedContext;
        List<JobType>           m_JobTypes;

        friend void WorkerThreadLoop(SharedContext* context);
    };

    // ---------------------------------------------------------------------------
    //							ThreadedVariable
    // ---------------------------------------------------------------------------

    template<typename T>
    class ThreadedVariable
    {
    public:
        ThreadedVariable(const T& value);
        ~ThreadedVariable() = default;

        void SetValue(const T& value);
        T    GetValue();

        void WaitUntilConditionMet(std::function<bool(const T&)> condition);

    private:
        T                   m_Variable;
        Mutex               m_Mutex;
        ConditionVariable	m_CV;
    };

    template<typename T>
    inline ThreadedVariable<T>::ThreadedVariable(const T& value)
    {
        m_Variable = value;
    }

    template<typename T>
    inline void ThreadedVariable<T>::SetValue(const T& value)
    {
        m_Mutex.lock();
        m_Variable = value;
        m_Mutex.unlock();
        m_CV.notify_all();
    }

    template<typename T>
    inline T ThreadedVariable<T>::GetValue()
    {
        m_Mutex.lock();
        const T& value = m_Variable;
        m_Mutex.unlock();

        return value;
    }

    template<typename T>
    inline void ThreadedVariable<T>::WaitUntilConditionMet(std::function<bool(const T&)> condition)
    {
        std::unique_lock<Mutex> lock(m_Mutex);

        while (!condition(m_Variable))
        {
            m_CV.wait(lock);
        }
    }
}
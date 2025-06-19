#include <gtest/gtest.h>
#include <RabBit/utils/Threading.h>

using namespace RB;

TEST(ThreadTest, SimpleWaitOnJob)
{
    int result = 0;
    
    struct Data : JobData
    {
        int* var;
    };
    Data* data = new Data();
    data->var = &result;
    
    auto job_test = [](JobData* data)
    {
        Sleep(500);

        Data* d = (Data*)data;
        *d->var = 1;
    };
    
    WorkerThread thread(L"test");
    thread.ScheduleJob(thread.AddJobType(job_test), data);

    thread.SyncAll();

    ASSERT_EQ(result, 1);
}

TEST(ThreadTest, SimpleWaitOnSpecificJob)
{
    int result = 0;
    
    struct Data : JobData
    {
        int* var;
    };
    Data* data = new Data();
    data->var = &result;
    
    auto job_test = [](JobData* data)
    {
        Sleep(500);

        Data* d = (Data*)data;
        *d->var = 1;
    };
    
    WorkerThread thread(L"test");
    JobID job = thread.ScheduleJob(thread.AddJobType(job_test), data);

    thread.Sync(job);

    ASSERT_EQ(result, 1);
}

TEST(ThreadTest, SimpleCheckFinished)
{
    auto job_test = [](JobData* data)
    {
        Sleep(500);
    };
    
    WorkerThread thread(L"test");
    JobID job = thread.ScheduleJob(thread.AddJobType(job_test), nullptr);

    ASSERT_EQ(thread.IsFinished(job), false);
    Sleep(1000);
    ASSERT_EQ(thread.IsFinished(job), true);
}

TEST(ThreadTest, SimpleStallCheck)
{
    auto job_test = [](JobData* data)
    {
        Sleep(750);
    };
    
    WorkerThread thread(L"test");
    thread.ScheduleJob(thread.AddJobType(job_test), nullptr);

    JobID job;
    ASSERT_EQ(thread.IsStalling(100, job), false);
    Sleep(150);
    ASSERT_EQ(thread.IsStalling(100, job), true);
}

TEST(ThreadTest, SimpleCancelTest)
{
    int result = 0;
    
    struct Data : JobData
    {
        int* var;
    };
    Data* data = new Data();
    data->var = &result;
    Data* data2 = new Data();
    data2->var = &result;
    
    auto job_test = [](JobData* data)
    {
        Sleep(500);

        Data* d = (Data*)data;
        *(d->var) = *(d->var) + 1;
    };
    
    WorkerThread thread(L"test");
    JobTypeID job_type = thread.AddJobType(job_test);
    JobID job1 = thread.ScheduleJob(job_type, data);
    JobID job2 = thread.ScheduleJob(job_type, data2);

    thread.Cancel(job2);
    thread.Sync(job1);

    ASSERT_EQ(result, 1);
}

TEST(ThreadTest, SimpleCancelAllTest)
{
    int result = 0;
    
    struct Data : JobData
    {
        int* var;
    };

    Data** datas = (Data**)alloca(sizeof(Data*) * 10);
    for (int i = 0; i < 10; i++)
    {
        datas[i] = new Data();
        datas[i]->var = &result;
    }
    
    auto job_test = [](JobData* data)
    {
        Sleep(500);

        Data* d = (Data*)data;
        *d->var = *d->var + 1;
    };
    
    WorkerThread thread(L"test");
    JobTypeID job_type = thread.AddJobType(job_test);

    for (int i = 0; i < 10; i++)
    {
        thread.ScheduleJob(job_type, datas[i]);
    }

    Sleep(50);
    thread.CancelAll();

    ASSERT_EQ(result, 1);
}

TEST(ThreadTest, SimpleJobOverwriteTest)
{
    int result = 0;

    struct Data : JobData
    {
        int* var;
    };
    Data** datas = (Data**)alloca(sizeof(Data*) * 5);
    for (int i = 0; i < 5; i++)
    {
        datas[i] = new Data();
        datas[i]->var = &result;
    }

    auto job_test = [](JobData* data)
    {
        Sleep(500);

        Data* d = (Data*)data;
        *d->var = *d->var + 1;
    };
    
    WorkerThread thread(L"test");
    JobTypeID job_type = thread.AddJobType(job_test, true);

    for (int i = 0; i < 5; i++)
    {
        thread.ScheduleJob(job_type, datas[i]);
    }

    thread.SyncAll();

    // Should be 2 because a job that is already running is not overwritten.
    // So we should end up with 1 running job and 1 in the queue.
    ASSERT_EQ(result, 2);
}

TEST(ThreadTest, PrioritizationTest)
{
    int result = 0;

    struct Data : JobData
    {
        int valueToAssign;
        int* var;
    };
    Data** datas = (Data**)alloca(sizeof(Data*) * 7);
    for (int i = 0; i < 7; i++)
    {
        datas[i] = new Data();
        datas[i]->var = &result;
    }
    datas[0]->valueToAssign = 1;
    datas[1]->valueToAssign = 4;
    datas[2]->valueToAssign = 5;
    datas[3]->valueToAssign = 2;
    datas[4]->valueToAssign = 6;
    datas[5]->valueToAssign = 3;
    datas[6]->valueToAssign = 7;

    auto job_test = [](JobData* jd)
    {
        Sleep(250);

        Data* data = (Data*)jd;
        
        int value = data->valueToAssign;
        int* result = data->var;

        ASSERT_EQ(*result, value - 1);

        *result = value;
    };

    WorkerThread thread(L"test");
    JobTypeID job_type = thread.AddJobType(job_test);

    for (int i = 0; i < 7; i++)
    {
        JobID job = thread.ScheduleJob(job_type, datas[i]);

        if (i == 3 || i == 5)
        {
            thread.PrioritizeJob(job);
        }
    }

    thread.SyncAll();
}

TEST(ThreadTest, PrioritizationSyncTest)
{
    // The same as the PrioritizationTest, but with some sync points

    ThreadedVariable<int> result = 0;

    struct Data : JobData
    {
        int valueToAssign;
        ThreadedVariable<int>* var;
    };
    Data** datas = (Data**)alloca(sizeof(Data*) * 7);
    for (int i = 0; i < 7; i++)
    {
        datas[i] = new Data();
        datas[i]->var = &result;
    }
    datas[0]->valueToAssign = 1;
    datas[1]->valueToAssign = 3;
    datas[2]->valueToAssign = 5;
    datas[3]->valueToAssign = 2;
    datas[4]->valueToAssign = 6;
    datas[5]->valueToAssign = 4;
    datas[6]->valueToAssign = 7;

    auto job_test = [](JobData* jd)
    {
        Sleep(450);

        Data* data = (Data*)jd;
        
        int value = data->valueToAssign;
        auto result = data->var;

        ASSERT_EQ(data->var->GetValue(), value - 1);

        data->var->SetValue(value);
    };

    WorkerThread thread(L"test1");
    WorkerThread thread2(L"test2");
    JobTypeID job_type = thread.AddJobType(job_test);

    JobID job0 = thread.ScheduleJob(job_type, datas[0]);
    JobID job1 = thread.ScheduleJob(job_type, datas[1]);
    JobID job2 = thread.ScheduleJob(job_type, datas[2]);
    JobID job3 = thread.ScheduleJob(job_type, datas[3]);
    thread.PrioritizeJob(job3);
    JobID job4 = thread.ScheduleJob(job_type, datas[4]);
    JobID job5 = thread.ScheduleJob(job_type, datas[5]);
    JobID job6 = thread.ScheduleJob(job_type, datas[6]);
    
    auto prio_job = [&thread, &job5](JobData* jd) mutable
    {
        Sleep(150);
        thread.PrioritizeJob(job5);
    };
    JobTypeID job_type2 = thread2.AddJobType(prio_job);

    thread.Sync(job3);
    ASSERT_EQ(result.GetValue(), 2);
    thread2.ScheduleJob(job_type2, nullptr);
    thread.Sync(job2);
    ASSERT_EQ(result.GetValue(), 5);
    thread.Sync(job6);
    ASSERT_EQ(result.GetValue(), 7);
}

TEST(ThreadTest, PrioritizationCancelTest)
{
    int result = 0;

    struct Data : JobData
    {
        int valueToAssign;
        int* var;
    };
    Data** datas = (Data**)alloca(sizeof(Data*) * 7);
    for (int i = 0; i < 7; i++)
    {
        datas[i] = new Data();
        datas[i]->var = &result;
    }
    datas[0]->valueToAssign = 1;
    datas[1]->valueToAssign = 3;
    datas[2]->valueToAssign = 11;
    datas[3]->valueToAssign = 2;
    datas[4]->valueToAssign = 4;
    datas[5]->valueToAssign = 10;
    datas[6]->valueToAssign = 5;

    auto job_test = [](JobData* jd)
    {
        Sleep(250);

        Data* data = (Data*)jd;
        
        int value = data->valueToAssign;
        int* result = data->var;

        ASSERT_EQ(*result, value - 1);

        *result = value;
    };

    WorkerThread thread(L"test");
    JobTypeID job_type = thread.AddJobType(job_test);

    for (int i = 0; i < 7; i++)
    {
        JobID job = thread.ScheduleJob(job_type, datas[i]);

        if (i == 3 || i == 5)
        {
            thread.PrioritizeJob(job);
        }
         
        if (i == 2 || i == 5)
        {
            thread.Cancel(job);
        }
    }

    thread.SyncAll();
}

TEST(ThreadTest, UltimatePrioritizationCancelSyncTest)
{
    
}
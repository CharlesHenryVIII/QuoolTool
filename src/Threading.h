#pragma once
#include "Math.h"

#include <atomic>
#include <mutex>
#include <semaphore>
#include <thread>
#include <vector>

struct Job
{
    virtual void RunJob() = 0;
};

struct Threading {
private:
    std::mutex                              m_jobVectorMutex;
    std::counting_semaphore<PTRDIFF_MAX>    m_semaphore;
    std::atomic<i32>                        m_jobsInFlight = {};
    std::atomic<bool>                       m_running;
    std::vector<Job*>                       m_jobs;
    std::vector<std::thread>                m_threads;

    static i32 ThreadFunction(void* data);
    Threading();
    ~Threading();
    Threading(Threading&) = delete;
    Threading& operator=(Threading&) = delete;
    Job* AcquireJob();

public:
    static Threading& GetInstance()
    {
        static Threading instance;
        return instance;
    }
    i32 GetJobsInFlight() const
    {
        return m_jobsInFlight;
    }
    void RunAndClearJobs()
    {
        while (true)
        {
            Job* job = AcquireJob();
            if (job == nullptr)
                return;

            m_semaphore.acquire();
            delete job;
            m_jobsInFlight--;
        }
    }
	void SubmitJob(Job* job);
};

bool OnMainThread();
#pragma once
#include "Math.h"

#include <atomic>
#include <mutex>
#include <semaphore>
#include <thread>
#include <vector>

using Mutex = std::mutex;
template<std::ptrdiff_t N>
using Semaphore = std::counting_semaphore<N>;
template<class N>
using Atomic = std::atomic<N>;

struct Job
{
    virtual void RunJob() = 0;
};

struct Threading {
private:
    Mutex                       m_jobVectorMutex;
    Semaphore<PTRDIFF_MAX>      m_semaphore;
    Atomic<i32>                 m_jobsInFlight = {};
    Atomic<bool>                m_running;
    std::vector<Job*>           m_jobs;
    std::vector<std::thread>    m_threads;

    struct ThreadData {
        std::string name;
        u32 index;
    };

    static i32 ThreadFunction(ThreadData data);
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
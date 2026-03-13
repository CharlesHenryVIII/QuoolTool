#pragma once
#include "Math.h"
#include "Debug.h"

#include "Tracy.hpp"

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
template<class N>
using Lock = std::lock_guard<N>;
//creates a lock_guard named "lock"
#define TRACY_LOCK(var) Lock<LockableBase(Mutex)> lock(var)
#define TRACY_MUTEX(var) TracyLockable(Mutex, var)

enum AsyncStatus : u32 {
    AsyncStatus_Empty,
    AsyncStatus_Fetching,
    AsyncStatus_FetchedSuccess,
    AsyncStatus_FetchedFailed,
    AsyncStatus_Count,
};
ENUMOPS_PURE(AsyncStatus);

template<typename T>
struct AsyncData {
    T data;
    TRACY_MUTEX(lock);
    Atomic<AsyncStatus> state = {};
};

struct Job
{
    virtual void RunJob() = 0;
};

struct Threading {
private:
    TRACY_MUTEX(m_jobVectorMutex);
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

template<class T> inline void operator^=(std::atomic<T>& a, T b)
{
    T at = a.load();
    at = T(+at ^ b);
    a.store(at);
}
template<class T> inline void operator|=(std::atomic<T>& a, T b)
{
    T at = a.load();
    at = T(+at | b);
    a.store(at);
}
template<class T> inline void operator&=(std::atomic<T>& a, T b)
{
    T at = a.load();
    at = T(+at & b);
    a.store(at);
}

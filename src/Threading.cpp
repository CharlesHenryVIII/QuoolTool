#include "Threading.h"
#include "Math.h"

#include <Chrono>

// NOTE(CSH): uncommenting this will enable asyncronous multithreading
// for the jobs however this does not work yet and i'm not sure if it will ever need to
//TODO: Make this work (maybe?)
//#define ASYNC

Threading::Threading()
    : m_semaphore(0)
{
#ifdef ASYNC
    u32 usableCores = Max<u32>(1, SDL_GetCPUCount() - 1);
#else
    u32 usableCores = 1;
#endif
    for (u32 i = 0; i < usableCores; ++i)
    {
        m_threads.push_back(std::thread(&Threading::ThreadFunction, nullptr));
    }
    m_running = true;
}
Threading::~Threading()
{
    m_running = false;
    m_semaphore.release(m_threads.size());

    for (std::thread& thread : m_threads)
        thread.join();
}

[[nodiscard]] Job* Threading::AcquireJob()
{

    std::lock_guard<std::mutex> lock(m_jobVectorMutex);
    if (m_jobs.empty())
        return nullptr;
    m_jobsInFlight++;

    Job* job = m_jobs[0];
    m_jobs.erase(m_jobs.begin());
    return job;
}

void Threading::SubmitJob(Job* job)
{
    std::lock_guard<std::mutex> lock(m_jobVectorMutex);
    m_jobs.push_back(job);
    m_semaphore.release();
}

i32 Threading::ThreadFunction(void* data)
{
    Threading& MT = GetInstance();

    while (true)
    {
        MT.m_semaphore.acquire();
        if (!MT.m_running)
            break;

        Job* job = MT.AcquireJob();
        ASSERT(job);
        if (job == nullptr)
            continue;

        job->RunJob();

        MT.m_jobsInFlight--;
        delete job;
    }
    return 0;
}

std::thread::id mainThreadID = std::this_thread::get_id();
bool OnMainThread()
{
    return mainThreadID == std::this_thread::get_id();
}
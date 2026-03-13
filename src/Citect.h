#pragma once
#include "Threading.h"
#include "String.h"

struct CitectData {
    Path project_path;
    Path program_files_path;
    Path program_files_86;
    Path backup_path;
    TRACY_MUTEX(lock);
    Atomic<bool> backup_in_progress = false;
    Atomic<u64> progress;
    Atomic<u64> total;
};

struct RunCitectFullBackupJob : Job
{
    CitectData* m_citect_data;
    virtual void RunJob() override;
};

struct RunCitectCreateZipJob : Job
{
    CitectData* m_citect_data;
    virtual void RunJob() override;
};

void CitectImGui(CitectData& citect_data);


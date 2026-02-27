#pragma once
#include "Threading.h"
#include "Settings.h"
#include "String.h"

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


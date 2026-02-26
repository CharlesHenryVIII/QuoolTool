#pragma once
#include "Threading.h"
#include "Settings.h"
#include "String.h"

struct RunCitectJob : Job
{
    CitectData* m_citect_data;
    virtual void RunJob() override;
};

void CitectImGui(CitectData& citect_data);


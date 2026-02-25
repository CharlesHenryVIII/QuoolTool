#pragma once
#include "Threading.h"
#include "Settings.h"
#include "String.h"

struct RunCitectJob : Job
{
    virtual void RunJob() override;
};

void CitectImGui();


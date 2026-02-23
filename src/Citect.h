#pragma once
#include "Threading.h"
#include "Settings.h"

#include <filesystem>

struct RunCitectJob : Job
{
    SettingsCitect settings;
    std::filesystem::path backup_path;
    virtual void RunJob() override;
};

void CitectImGui();


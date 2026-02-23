#pragma once
#include "Math.h"
#include "Themes.h"

#include <filesystem>
#include <mutex>

struct Track {
    std::string type;
    std::string details;
    i32 id;
    bool encode = false;
};

struct VideoInfo {
    std::wstring name;
    std::vector<Track> tracks;
    bool encode = true;
};

struct VideoGroup {
    std::vector<VideoInfo> video_infos;

    std::mutex thread_lock;
    i32 max_tracks = 0;
    std::atomic<bool> in_progress;
    std::atomic<bool> stop = false;
    std::atomic<i32> completed;

    bool Clear()
    {

        if (thread_lock.try_lock())
        {
            Defer{ thread_lock.unlock(); };

            if (in_progress)
                return false;

            video_infos.clear();
            return true;
        }
        return false;
    }
};

struct SettingsCitect {
    std::wstring project_path;
    std::wstring program_files_path;
};

struct Settings {
    SettingsCitect citect = {};
    std::filesystem::path backup_path;
    ThemeColor color = ThemeColor_Grey2;
    ThemeStyle style = ThemeStyle_SimpleRounding;
};

struct GlobalData
{
    Settings settings;
    std::atomic<bool> backup_in_progress = false;
    std::atomic<u64> progress;
    std::atomic<u64> total;
};

extern GlobalData g_data;

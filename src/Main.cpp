#include "CashUtil/CashUtil.h"

#include "Tracy.hpp"
#include "cmdline.h"

#include "Settings.h"
#include "LoadJson.h"
#include "ImguiHelper.h"
#include "resource.h"
#include "Citect.h"
#include "Networking.h"
#include "DataCollection.h"
#include "stb/stb_image.h"

#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

static const std::string s_logo =
R"(  ________  ___  ___  ________  ________  ___               _________  ________  ________  ___          \n)"
R"( |\   __  \|\  \|\  \|\   __  \|\   __  \|\  \             |\___   ___\\   __  \|\   __  \|\  \         \n)"
R"( \ \  \|\  \ \  \\\  \ \  \|\  \ \  \|\  \ \  \            \|___ \  \_\ \  \|\  \ \  \|\  \ \  \        \n)"
R"(  \ \  \\\  \ \  \\\  \ \  \\\  \ \  \\\  \ \  \                \ \  \ \ \  \\\  \ \  \\\  \ \  \       \n)"
R"(   \ \  \\\  \ \  \\\  \ \  \\\  \ \  \\\  \ \  \____            \ \  \ \ \  \\\  \ \  \\\  \ \  \____  \n)"
R"(    \ \_____  \ \_______\ \_______\ \_______\ \_______\           \ \__\ \ \_______\ \_______\ \_______\\n)"
R"(     \|___| \__\|_______|\|_______|\|_______|\|_______|            \|__|  \|_______|\|_______|\|_______|\n)"
R"(           \|__|                                                                                        \n)"
R"(\n)";

i32 SysMain(i32 argc, char** argv)
{
    DebugPrint("====================");
    DebugPrint("     Quool Tool     ");
    DebugPrint("====================\n");
#if 0
    //ERROR: Threading will not work correctly unless InitOS() is called before
    //argc will always be 1 in console mode
    if (**argv != 0 && (argc == -1 || argc > 1))
    {
        cmdline::parser p;
        const char* backup_name = "backup-path";
        const char* citect_project_path_name = "citect-project-path";
        const char* citect_program_files_name = "citect-program-files";
        p.add<std::string>(backup_name, 'b', "path to backup folder", false, "");
        p.add<std::string>(citect_project_path_name, 'c', "path to citect project to zip, please also provide citect-program-files path if available", false, "");
        p.add<std::string>(citect_program_files_name, 'p', "path to citect program files to zip, must provide citect-project-path as well", false, "");
        if (argc == -1)
            p.parse_check(*argv);
        else
            p.parse_check(argc, argv);

        const std::string& backup_path = p.get<std::string>(backup_name);
        const std::string& citect_project_folder_string = p.get<std::string>(citect_project_path_name);
        const std::string& citect_program_files_string = p.get<std::string>(citect_program_files_name);
        if (citect_project_folder_string.size())
        {
            Path project = citect_project_folder_string;
            Path program_files = citect_program_files_string;
            if (fs::exists(project))
            {
                //TODO: Do Backup

                CitectData cd = {
                    .project_path = project,
                    .program_files_path = fs::exists(program_files) ? program_files : Path(),
                    .backup_path = backup_path,
                };
                RunCitectCreateZipJob* job = new RunCitectCreateZipJob();
                job->m_citect_data = &cd;
                Threading::GetInstance().SubmitJob(job);
                while (cd.total == 0)
                {
                    SysSleep(200);
                }

                do {
                    TuiProgressBar(cd.progress, cd.total);
                    SysSleep(100);
                } while (cd.total != 0);
                TuiProgressBar(100, 100);
            }
            else
            {
                std::cerr << "folder does not exist for citect-zip: " << citect_project_folder_string;
            }
        }
        return 0;
    }
    HideConsole();
#endif

    ReadSettings();

    ArrayView<const u8> embedded_images[IDB_PNGEND - IDB_PNGFULL] = {};
    for (i32 icon_id = IDB_PNGFULL; icon_id < IDB_PNGEND; icon_id++)
    {
        i32 size;
        const u8* data = (u8*)SysGetDataFromResource(&size, icon_id);
        if (!data)
        {
            DebugPrint("Error: failed to get data from resource: %i", icon_id);
            FAIL;
            continue;
        }
        embedded_images[icon_id - IDB_PNGFULL] = CreateArrayView(data, size);
    }
    if (!RenderInit(CreateArrayView(embedded_images)))
    {
        return 1;
    }
    if (!SysInit(gfx.window))
    {
        DebugPrint("Error: OSInit() failed");
    }
    Threading& threading = Threading::GetInstance();
    AppData app_data = {};
    DataCollectionInit(&app_data.data_collection_data);
    NetworkingInit(&app_data.network_data);
    CitectInit(&app_data.citect_data);
    ImguiInit();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGuiStyle& style = ImGui::GetStyle();
    ThemesInit(g_theme_settings.color, g_theme_settings.style, ThemeColor_Quantum, ThemeStyle_SimpleRounding);
    //ConsoleInit(s_logo);



    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details. If you like the default font but want it to scale better, consider using the 'ProggyVector' from the same author!
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    style.FontSizeBase = 16.0f;
    g_data.fonts[FontIndex_Default] = SysLoadFontForImgui(IDR_FONT1, 16.0f);
    g_data.fonts[FontIndex_Small] = SysLoadFontForImgui(IDR_FONT1, 12.0f);
    g_data.fonts[FontIndex_Imgui] = io.Fonts->AddFontDefault();
    g_data.fonts[FontIndex_Monospace] = SysLoadFontForImgui(IDR_FONT2, 16.0f);

    // Main loop
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (g_running)
#endif
    {
        {
            ZoneScopedN("Frame Update:");
            ++g_frame_index;
            SysProcessEvents();

#if _DEBUG
            if (g_sysinfo.keys[SDLK_ESCAPE].downThisFrame)
                g_running = false;
#endif

            ImguiNewFrame();
            ImguiMain(app_data);

            {
                ZoneScopedN("ImGui Render");
                {
                    ZoneScopedN("ImGui Render");
                    ImGui::Render();
                }
                RenderPresent();
            }
        }

        FrameMark;
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    //(Threading::GetInstance()).ForceQuit();
    ImguiDestroy();
    DataCollectionDestroy(&app_data.data_collection_data);
    NetworkingDestroy(&app_data.network_data);
    CitectDestroy(&app_data.citect_data);
    RenderDestroy();
    SysDestroy(gfx.window);
    SDL_Quit();

    return 0;
}
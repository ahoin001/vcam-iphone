#pragma once

#include <string>

namespace vcam {

struct UpdateOffer {
    bool ok = false;
    bool available = false;
    std::string tag;
    std::string url;
    std::wstring detail;
};

struct UpdateStage {
    bool ok = false;
    std::wstring folder;
    std::wstring detail;
};

const char* CurrentVersion();
bool IsDevBuild();
UpdateOffer CheckLatest();
UpdateStage PrepareUpdate(const std::string& url);
bool LaunchUpdater(const std::wstring& stageFolder);
std::wstring ExeFolder();

}  // namespace vcam

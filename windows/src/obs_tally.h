#pragma once

#include <string>

// Watches OBS on this PC and forwards program / preview / idle to the phone tally badge.
void StartObsTally(const std::string& host, int port, const std::wstring& password);
void StopObsTally();

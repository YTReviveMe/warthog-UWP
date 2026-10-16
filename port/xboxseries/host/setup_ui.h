#pragma once

#include <filesystem>
#include <string>

bool xbox_show_setup_ui(const std::filesystem::path &local_root, std::filesystem::path &image,
                        std::filesystem::path &destination, std::string &error);

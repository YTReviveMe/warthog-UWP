#pragma once

#include <filesystem>

bool xbox_install_game_data(const std::filesystem::path &local_root, std::filesystem::path &installed_root);

bool xbox_restore_game_data(const std::filesystem::path &local_root, std::filesystem::path &installed_root);

void xbox_remember_game_data(const std::filesystem::path &local_root, const std::filesystem::path &installed_root);

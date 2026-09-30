#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <string_view>
#include <print>
#include "../required_libraries/rapidjson/document.h"

void showerr(std::string_view error) {
    std::println("\x1b[91mcentrik.glslbuilder:\x1b[0m {}", error);
    std::_Exit(1);
}

void showsuc(std::string_view success) {
    std::println("\x1b[92mcentrik.glslbuilder:\x1b[0m {}", success);
}

void showinf(std::string_view info) {
    std::println("\x1b[94mcentrik.glslbuilder:\x1b[0m {}", info);
}

auto main(int argc, char* argv[]) -> int {
    auto gitters_path = std::filesystem::path(".gitters.json");
    if (!std::filesystem::exists(gitters_path))
        showerr(".gitters.json does not exist");
    if (!std::filesystem::is_regular_file(gitters_path))
        showerr(".gitters.json is not a regular file");

    std::ifstream gitters_file(gitters_path);
    std::string shaders_content((std::istreambuf_iterator<char>(gitters_file)), std::istreambuf_iterator<char>());

    rapidjson::Document doc;
    doc.Parse(shaders_content.c_str());
    if (doc.HasParseError()) showerr("failed to parse .gitters.json");
    
    if (!doc.HasMember("files")) showerr("files not found in .gitters.json");
    if (!doc["files"].IsArray()) showerr("files is not an array in .gitters.json");
    for (auto& file : doc["files"].GetArray()) {
        if (!file.IsString()) showerr("file is not a string in .gitters.json");
        std::string_view file_path = std::string_view(file.GetString(), file.GetStringLength());
        std::string command = std::format("git add {}", file_path);

        showinf(std::format("running command: {}", command));
        if (int ec = std::system(command.c_str()))
            showerr(std::format("failed to run command, exit code: {}", ec));
        showsuc(std::format("ran command successfully"));
    }

    std::string save_command = std::format("git add -u");
    showinf(std::format("running command: {}", save_command));
    if (int ec = std::system(save_command.c_str()))
        showerr(std::format("failed to run command, exit code: {}", ec));
    showsuc(std::format("ran command successfully"));

    std::string_view commit_message = "commit";
    if (argc > 1) commit_message = argv[1];
    std::string commit_command = std::format("git commit -m {}", commit_message);
    showinf(std::format("running command: {}", commit_command));
    if (int ec = std::system(commit_command.c_str()))
        showerr(std::format("failed to run command, exit code: {}", ec));
    showsuc(std::format("ran command successfully"));
}
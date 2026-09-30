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
    if (argc < 1) showerr("no commands input");
    std::string_view mode = std::string_view(argv[1]);
    if (mode == "push") {
        bool force = false;
        std::string_view branch = "main";
        for (int i = 2; i < argc; ++i) {
            if (std::string_view(argv[i]) == "-force") force = true;
            else branch = std::string_view(argv[i]);
        }
        
        std::string push_command = std::format("git push {} -u origin {}", force ? "-f" : "", branch);
        showinf(std::format("running command: {}", push_command));
        if (int ec = std::system(push_command.c_str()))
            showerr("failed to push");
        showsuc("push successful");
        return 0;
    } else if (mode != "mit") showerr("invalid mode");
    
    auto gitters_path = std::filesystem::path(".gitmdocs.json");
    if (!std::filesystem::exists(gitters_path))
        showerr(".gitmdocs.json does not exist");
    if (!std::filesystem::is_regular_file(gitters_path))
        showerr(".gitmdocs.json is not a regular file");

    std::ifstream gitters_file(gitters_path);
    std::string shaders_content((std::istreambuf_iterator<char>(gitters_file)), std::istreambuf_iterator<char>());

    rapidjson::Document doc;
    doc.Parse(shaders_content.c_str());
    if (doc.HasParseError()) showerr("failed to parse .gitmdocs.json");

    if (!doc.HasMember("files")) showerr("files not found in .gitmdocs.json");
    if (!doc["files"].IsArray()) showerr("files is not an array in .gitmdocs.json");
    for (auto& file : doc["files"].GetArray()) {
        if (!file.IsString()) showerr("file is not a string in .gitmdocs.json");
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

    for (int i = 1; i < argc; i++) {
        if (std::string_view(argv[i]) == "-push") {
            std::string push_command = std::format("git push");
            showinf(std::format("running command: {}", push_command));
            if (int ec = std::system(push_command.c_str()))
                showerr(std::format("failed to run command, exit code: {}", ec));
            showsuc(std::format("ran command successfully"));
            break;
        }
    }
}

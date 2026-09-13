#define RYML_SINGLE_HDR_DEFINE_NOW
#include "../required_libraries/ryml.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <print>
#include <string_view>
#include <regex>

void showerr(std::string_view error) {
    std::println("\x1b[91mcentrik.clangdbuilder:\x1b[0m {}", error);
    std::_Exit(1);
}

void showsuc(std::string_view success) {
    std::println("\x1b[92mcentrik.clangdbuilder:\x1b[0m {}", success);
}

void showinf(std::string_view info) {
    std::println("\x1b[94mcentrik.clangdbuilder:\x1b[0m {}", info);
}

int main(int argc, char* argv[]) {
    std::filesystem::path clangd_path = std::filesystem::current_path() / ".clangd";
    if (!std::filesystem::exists(clangd_path))
        showerr(".clangd file doesn't exists");

    std::ifstream clangd_file(clangd_path);
    std::string clangd_content((std::istreambuf_iterator<char>(clangd_file)), std::istreambuf_iterator<char>());

    ryml::Tree tree = ryml::parse_in_arena(clangd_content.c_str());
    ryml::ConstNodeRef root = tree.rootref();
    if (root.invalid() || !root.is_map())
        showerr("top-level YAML is not a map");
    auto cf_node = tree["CompileFlags"];
    if (cf_node.invalid()) showerr("CompileFlags not found");
    if (!cf_node.is_map()) showerr("CompileFlags is not a map");
    auto add_node = cf_node["Add"];
    if (add_node.invalid()) showerr("CompileFlags/Add not found");
    if (!add_node.is_seq()) showerr("CompileFlags/Add is not a sequence");

    std::string compile_command = "";
    for (ryml::ConstNodeRef item : add_node) {
        std::string cmd = std::string(item.val().str, item.val().len);
        compile_command += cmd + " ";
    }
    showinf("compile command: " + compile_command);

    int ec = std::system(compile_command.c_str());
    if (ec) showerr("failed to compile program, exit code: " + std::to_string(ec));

    std::string output_file = ""; std::smatch match;
    if (!std::regex_search(compile_command, match, std::regex(R"(\-o\s+(\S+))")))
        showerr("no output file specified");
    output_file = match[1].str();
    output_file = std::filesystem::absolute(output_file).string();
    showsuc("successfully compiled program, output file: " + output_file);

    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "-run") {
            showinf("running program: " + output_file);
            ec = std::system(("powershell -Command " + output_file).c_str());
            if (ec) showerr("failed to run program, exit code: " + std::to_string(ec));
            showsuc("successfully ran program");
            return 0;
        }
    }
}

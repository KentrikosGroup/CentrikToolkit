#include <vector>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <print>
#include <string_view>
#include <unordered_map>
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

void inline_shaders(std::string_view shader_path) {
    auto path = std::filesystem::path(shader_path);
    if (!std::filesystem::exists(path))
        showerr("file " + std::string(shader_path) + " does not exist");
    if (!std::filesystem::is_regular_file(path))
        showerr("file " + std::string(shader_path) + " is not a regular file");

    std::ifstream shader_file(path, std::ios::binary);
    std::string shader((std::istreambuf_iterator<char>(shader_file)), std::istreambuf_iterator<char>());

    std::filesystem::path out_path = path;
    out_path.replace_extension(".inl");

    std::ofstream shader_out(out_path);
    if (!shader_out) showerr("cannot create file: " + out_path.string());

    std::string varname = path.stem().string();
    for (auto& c : varname) if (!std::isalnum(c)) c = '_';

    shader_out << std::format("static const unsigned char {}[] = {{\n", varname);
    for (size_t i = 0; i < shader.size(); i++) {
        if (i % 16 == 0) shader_out << "    ";
        shader_out << std::format("0x{:02x}", static_cast<unsigned char>(shader[i]));
        if (i + 1 < shader.size()) shader_out << ", ";
        if ((i + 1) % 16 == 0) shader_out << "\n";
    }
    shader_out << "\n};\n";
    shader_out << std::format("static constexpr size_t {}_len = {};\n", varname, shader.size());

    showinf("generated inline file: " + out_path.string());
}

int main(int argc, char* argv[]) {
    std::filesystem::path clangd_path = std::filesystem::current_path() / ".shaders.json";
    if (!std::filesystem::exists(clangd_path))
        showerr(".shaders.json file doesn't exists");

    std::ifstream shaders_file(clangd_path);
    std::string shaders_content((std::istreambuf_iterator<char>(shaders_file)), std::istreambuf_iterator<char>());

    rapidjson::Document doc;
    doc.Parse(shaders_content.c_str());
    if (doc.HasParseError()) showerr("failed to parse .shaders.json");

    std::string_view glslc_path = "";
    if (!doc.HasMember("glslc")) showerr("glslc path not found in .shaders.json");
    if (!doc["glslc"].IsString()) showerr("glslc path is not a string in .shaders.json");
    glslc_path = std::string_view(doc["glslc"].GetString(), doc["glslc"].GetStringLength());

    if (!doc.HasMember("shaders")) showerr("shaders not found in .shaders.json");
    if (!doc["shaders"].IsArray()) showerr("shaders is not an array in .shaders.json");

    static const std::unordered_map<std::string, std::string> stage_map = {
        {".vert",  "-fshader-stage=vertex "},
        {".frag",  "-fshader-stage=fragment "},
        {".geom",  "-fshader-stage=geometry "},
        {".tesc",  "-fshader-stage=tesscontrol "},
        {".tese",  "-fshader-stage=tesseval "},
        {".comp",  "-fshader-stage=compute "},
        {".mesh",  "-fshader-stage=mesh "},
        {".rchit", "-fshader-stage=raytracing "},
        {".rmiss", "-fshader-stage=raytracing "},
        {".rcall", "-fshader-stage=raytracing "},
        {".rgen",  "-fshader-stage=raytracing "},
        {".rdepth","-fshader-stage=raytracing "},
    };

    std::vector<std::string> output_shaders;
    for (auto& shader : doc["shaders"].GetArray()) {
        if (!shader.IsString()) showerr("shader is not a string in .shaders.json");
        std::filesystem::path shader_path = std::string_view(shader.GetString(), shader.GetStringLength());
        std::string compile_command = std::string(glslc_path) + " ";

        std::string suffix = shader_path.extension().string();
        auto it = stage_map.find(suffix);
        if (it != stage_map.end()) compile_command += it->second;
        else showerr("unknown suffix: " + suffix);
        std::string shader_p = shader_path.string();
        std::string output_file = shader_path.replace_extension(".spv").string();
        compile_command += shader_p + " -o " + output_file;

        showinf("compile command: " + compile_command);
        int ec = std::system(compile_command.c_str());
        if (ec != 0) showerr("failed to compile shader: " + shader_path.string() + ", exit code: " + std::to_string(ec));
        showsuc("successfully compiled shader: " + shader_path.string());
        output_shaders.push_back(output_file);
    }

    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) != "-inline")
            continue;
        
        for (auto& shader : output_shaders)
            inline_shaders(shader);
        return 0;
    }
}

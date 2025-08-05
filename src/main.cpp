#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "glslang/MachineIndependent/localintermediate.h"
#include "glslang/SPIRV/GlslangToSpv.h"
#include "glslang/Public/ShaderLang.h"
#include "glslang/Public/ResourceLimits.h"
#include "glslang/Include/intermediate.h"

// From glslang/StandAlone/StandAlone.cpp
// Helper to get the shader stage from the file extension.
EShLanguage FindLanguage(const std::string& name)
{
    size_t dot = name.find_last_of('.');
    if (dot == std::string::npos)
        return EShLangVertex;

    std::string ext = name.substr(dot + 1);
    if (ext == "vert")
        return EShLangVertex;
    else if (ext == "tesc")
        return EShLangTessControl;
    else if (ext == "tese")
        return EShLangTessEvaluation;
    else if (ext == "geom")
        return EShLangGeometry;
    else if (ext == "frag")
        return EShLangFragment;
    else if (ext == "comp")
        return EShLangCompute;
    else if (ext == "rgen")
        return EShLangRayGen;
    else if (ext == "rint")
        return EShLangIntersect;
    else if (ext == "rahit")
        return EShLangAnyHit;
    else if (ext == "rchit")
        return EShLangClosestHit;
    else if (ext == "rmiss")
        return EShLangMiss;
    else if (ext == "rcall")
        return EShLangCallable;

    return EShLangVertex;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <shader_file> [output_file]" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    std::string outfilename;

    if (argc > 2) {
        outfilename = argv[2];
    } else {
        outfilename = filename.substr(0, filename.find_last_of('.')) + ".spv";
    }

    // Initialize glslang
    glslang::InitializeProcess();

    // Read shader file
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        glslang::FinalizeProcess();
        return 1;
    }
    std::string shaderCode((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    const char* shaderStrings[1];
    shaderStrings[0] = shaderCode.c_str();

    EShLanguage stage = FindLanguage(filename);
    glslang::TShader shader(stage);
    shader.setStrings(shaderStrings, 1);

    // Set up environment
    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 100);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_1);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);

    EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

    const int defaultVersion = 100;

    // Parse
    if (!shader.parse(GetDefaultResources(), defaultVersion, false, messages)) {
        std::cerr << "Shader parsing failed:" << std::endl;
        std::cerr << shader.getInfoLog() << std::endl;
        std::cerr << shader.getInfoDebugLog() << std::endl;
        glslang::FinalizeProcess();
        return 1;
    }

    // Link the shader into a program
    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(messages)) {
        std::cerr << "Shader linking failed:" << std::endl;
        std::cerr << program.getInfoLog() << std::endl;
        std::cerr << program.getInfoDebugLog() << std::endl;
        glslang::FinalizeProcess();
        return 1;
    }

    // Convert to SPIR-V
    std::vector<unsigned int> spirv;
    glslang::GlslangToSpv(*program.getIntermediate(stage), spirv);

    // Write SPIR-V to file
    std::ofstream outfile(outfilename, std::ios::binary);
    outfile.write((const char*)spirv.data(), spirv.size() * sizeof(unsigned int));
    outfile.close();

    std::cout << "SPIR-V code exported to " << outfilename << std::endl;

    // Finalize glslang
    glslang::FinalizeProcess();

    return 0;
}

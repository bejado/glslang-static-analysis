#include <iostream>
#include <fstream>
#include <string>

#include "glslang/MachineIndependent/localintermediate.h"
#include "glslang/Public/ShaderLang.h"
#include "glslang/Public/ResourceLimits.h"
#include "glslang/Include/intermediate.h"

// A simple AST traverser that prints the structure of the AST.
class AstPrinter : public glslang::TIntermTraverser {
public:
    AstPrinter(int indent = 0) : TIntermTraverser(true, false, false), mIndent(indent) {}

    void visitSymbol(glslang::TIntermSymbol* node) override {
        printIndent();
        std::cout << "Symbol: " << node->getName().c_str() << std::endl;
    }

    bool visitBinary(glslang::TVisit visit, glslang::TIntermBinary* node) override {
        printIndent();
        std::cout << "Binary Op: " << node->getOp() << std::endl;
        mIndent++;
        TIntermTraverser::visitBinary(visit, node);
        mIndent--;

        return true;
    }

    bool visitUnary(glslang::TVisit visit, glslang::TIntermUnary* node) override {
        printIndent();
        std::cout << "Unary Op: " << node->getOp() << std::endl;
        mIndent++;
        TIntermTraverser::visitUnary(visit, node);
        mIndent--;

        return true;
    }

    bool visitAggregate(glslang::TVisit visit, glslang::TIntermAggregate* node) override {
        printIndent();
        std::cout << "Aggregate: " << node->getOp() << std::endl;
        mIndent++;
        TIntermTraverser::visitAggregate(visit, node);
        mIndent--;

        return true;
    }

    void visitConstantUnion(glslang::TIntermConstantUnion* node) override {
        printIndent();
        std::cout << "Constant: ";
        switch (node->getBasicType()) {
            case glslang::EbtFloat:
            case glslang::EbtDouble:
                std::cout << node->getConstArray()[0].getDConst();
                break;
            case glslang::EbtInt:
                std::cout << node->getConstArray()[0].getIConst();
                break;
            case glslang::EbtBool:
                std::cout << node->getConstArray()[0].getBConst();
                break;
            default:
                std::cout << "(unhandled type)";
        }
        std::cout << std::endl;
    }

    bool visitSelection(glslang::TVisit, glslang::TIntermSelection* node) override {
        printIndent();
        std::cout << "If" << std::endl;
        mIndent++;
        printIndent();
        std::cout << "Condition:" << std::endl;
        mIndent++;
        node->getCondition()->traverse(this);
        mIndent--;

        if (node->getTrueBlock()) {
            printIndent();
            std::cout << "True Block:" << std::endl;
            mIndent++;
            node->getTrueBlock()->traverse(this);
            mIndent--;
        }

        if (node->getFalseBlock()) {
            printIndent();
            std::cout << "False Block:" << std::endl;
            mIndent++;
            node->getFalseBlock()->traverse(this);
            mIndent--;
        }
        mIndent--;
        return false; // we traversed the children ourselves
    }

    bool visitBranch(glslang::TVisit visit, glslang::TIntermBranch* node) override {
        printIndent();
        std::cout << "Branch: " << node->getFlowOp() << std::endl;

        return true;
    }

    bool visitLoop(glslang::TVisit visit, glslang::TIntermLoop* node) override {
        printIndent();
        std::cout << "Loop" << std::endl;
        mIndent++;
        if(node->getTest()) {
            printIndent();
            std::cout << "Test:" << std::endl;
            mIndent++;
            node->getTest()->traverse(this);
            mIndent--;
        }
        if(node->getBody()) {
            printIndent();
            std::cout << "Body:" << std::endl;
            mIndent++;
            node->getBody()->traverse(this);
            mIndent--;
        }
        if(node->getTerminal()) {
            printIndent();
            std::cout << "Terminal:" << std::endl;
            mIndent++;
            node->getTerminal()->traverse(this);
            mIndent--;
        }
        mIndent--;

        return true;
    }


private:
    void printIndent() {
        for (int i = 0; i < mIndent; ++i) {
            std::cout << "  ";
        }
    }
    int mIndent;
};


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
        std::cerr << "Usage: " << argv[0] << " <shader_file>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];

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

    // Get AST root
    TIntermNode* root = shader.getIntermediate()->getTreeRoot();
    if (!root) {
        std::cerr << "Failed to get AST root." << std::endl;
        glslang::FinalizeProcess();
        return 1;
    }

    // Print AST
    std::cout << "AST for " << filename << ":" << std::endl;
    AstPrinter printer;
    root->traverse(&printer);

    // Finalize glslang
    glslang::FinalizeProcess();

    return 0;
}

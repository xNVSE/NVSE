#pragma once
#include <format>

#include "NVSECompilerUtils.h"
#include "NVSELexer.h"
#include "ScriptTokens.h"

class NVSEScope {
public:
    struct ScopeVar {
        Token_Type detailedType;
        Script::VariableType scriptType;
        
        NVSEToken token;
        uint32_t index;
        uint32_t scopeIndex;
        
        bool global;

        // Used for renaming global variables in certain scopes
        //      export keyword will NOT rename a global var
        //      fn (int x) ... WILL rename x, so that x can be reused for other lambdas
        std::string rename {};

        std::string GetName() {
            return rename.empty() ? token.lexeme : rename;
        }
    };

private:
    uint32_t scopeIndex{};
    uint32_t varIndex{1};
    std::shared_ptr<NVSEScope> parent{};
    std::unordered_map<std::string, ScopeVar> vars{};
    bool isLambda{};

protected:
    uint32_t getVarIndex() {
        return varIndex;
    }

    void incrementVarIndex() {
        varIndex++;
    }
    
public:
    NVSEScope (uint32_t scopeIndex, std::shared_ptr<NVSEScope> parent, bool isLambda = false) : scopeIndex(scopeIndex), parent(parent), isLambda(isLambda) {
        if (parent) {
            varIndex = parent->getVarIndex();
        }
    }
    
    ScopeVar *resolveVariable(std::string name, bool checkParent = true) {
        if (vars.contains(name)) {
            return &vars[name];
        }

        if (checkParent && parent) {
            const auto var = parent->resolveVariable(name);
            if (!var || (isLambda && !var->global)) {
                return nullptr;
            }

            return var;
        }

        return nullptr;
    }

    ScopeVar *addVariable(std::string name, ScopeVar variableInfo, bool bRename = false) {
        // Rename var to have a unique name
        // These should also get saved to the var list LAST and be deleted on every recompile.
        if (bRename) {
            variableInfo.rename = std::format("__global__{}__{}", name, getVarIndex());
        } else {
            variableInfo.rename = "";
        }

        variableInfo.index = getVarIndex();
        if (!variableInfo.global) {
            incrementVarIndex();
        }

        variableInfo.scopeIndex = scopeIndex;
        
        vars[name] = variableInfo;
        return &vars[name];
    }

    bool isRootScope() const {
        return parent == nullptr;
    }

    uint32_t getScopeIndex() const {
        return scopeIndex;
    }
};

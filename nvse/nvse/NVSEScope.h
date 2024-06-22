#pragma once
#include <format>

#include "NVSECompilerUtils.h"
#include "NVSELexer.h"
#include "ScriptTokens.h"

class NVSEScope {
public:
    struct ScopeVar {
        Token_Type detailedType;
        Script::VariableType variableType;
        
        NVSEToken token;
        uint32_t index;
        uint32_t scopeIndex;
        
        bool isGlobal = false;

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

protected:
    uint32_t getVarIndex() const {
        if (parent) {
            return parent->getVarIndex();
        }
        
        return varIndex;
    }

    void incrementVarIndex() {
        if (parent) {
            parent->incrementVarIndex();
            return;
        }
        
        varIndex++;
    }
    
public:
    std::map<uint32_t, std::string> allVars{};
    
    NVSEScope (uint32_t scopeIndex, std::shared_ptr<NVSEScope> parent, bool isLambda = false)
        : scopeIndex(scopeIndex), parent(parent) {}
    
    ScopeVar *resolveVariable(std::string name, bool checkParent = true) {
        // Lowercase name
        std::ranges::transform(name, name.begin(), [](unsigned char c){ return std::tolower(c); });
        
        if (vars.contains(name)) {
            return &vars[name];
        }

        if (checkParent && parent) {
            const auto var = parent->resolveVariable(name);
            if (!var) {
                return nullptr;
            }

            return var;
        }

        return nullptr;
    }

    ScopeVar *addVariable(std::string name, ScopeVar variableInfo, bool bRename = false) {
        // Lowercase name
        std::ranges::transform(name, name.begin(), [](unsigned char c){ return std::tolower(c); });
        
        // Rename var to have a unique name
        // These should also get saved to the var list LAST and be deleted on every recompile.
        if (bRename) {
            variableInfo.rename = std::format("__temp__{}__{}", name, getVarIndex());
        } else {
            variableInfo.rename = "";
        }

        variableInfo.index = getVarIndex();
        if (!variableInfo.isGlobal) {
            incrementVarIndex();
        }

        variableInfo.scopeIndex = scopeIndex;
        
        vars[name] = variableInfo;
        addToAllVars(variableInfo);
        return &vars[name];
    }

    void addToAllVars(ScopeVar var) {
        if (parent) {
            parent->addToAllVars(var);
            return;
        }

        allVars[var.index] = var.token.lexeme;
    }

    bool isRootScope() const {
        return parent == nullptr;
    }

    uint32_t getScopeIndex() const {
        return scopeIndex;
    }
};

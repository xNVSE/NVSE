#pragma once
#include "NVSELexer.h"
#include "ScriptTokens.h"

class NVSEScope {
public:
    struct ScopeVar {
        Token_Type detailedType;
        NVSEToken token;
        uint32_t index;
        uint32_t scopeIndex;
    };

private:
    uint32_t scopeIndex{};
    uint32_t varIndex{};
    std::shared_ptr<NVSEScope> parent{};
    std::unordered_map<std::string, ScopeVar> vars{};

protected:
    uint32_t getVarIndex() {
        if (!parent) {
            return varIndex;
        }
        
        return parent->varIndex;
    }

    void incrementVarIndex() {
        if (!parent) {
            varIndex++;
            return;
        }
        
        parent->incrementVarIndex();
    }
    
public:
    NVSEScope (uint32_t scopeIndex, std::shared_ptr<NVSEScope> parent) : scopeIndex(scopeIndex), parent(parent) {}
    NVSEScope (uint32_t scopeIndex) : scopeIndex(scopeIndex), parent(nullptr) {}
    
    ScopeVar *resolveVariable(std::string name, bool checkParent = true) {
        if (vars.contains(name)) {
            return &vars[name];
        }

        if (checkParent && parent) {
            return parent->resolveVariable(name);
        }

        return nullptr;
    }

    void addVariable(std::string name, ScopeVar variableInfo) {
        variableInfo.index = getVarIndex();
        incrementVarIndex();

        variableInfo.scopeIndex = scopeIndex;
        
        vars[name] = variableInfo;
    }

    bool isRootScope() const {
        return parent == nullptr;
    }

    uint32_t getScopeIndex() const {
        return scopeIndex;
    }
};

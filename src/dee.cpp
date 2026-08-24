#include "dee.h"

// contextId encoders: pack an instance id and its ContextType into a single int, so a
// faction/city-scoped dependency can never collide with WORLD_CTX or with an
// instance of the OTHER context type that happens to share the same numeric id (e.g.
// faction 0 and city 0 get different contextIds). ContextType (2 bits: 0/1/2) is the tag in
// the low bits, the instance id is shifted into the high bits.
int worldContext() { return WORLD_CTX; }
int factionContext(int factionId) { return (factionId << 2) | FACTION_CTX; }
int cityContext(int cityId) { return (cityId << 2) | CITY_CTX; }

DependencyEvaluationEngine::DependencyEvaluationEngine()
{
    // Constructor implementation
}


void DependencyEvaluationEngine::regDep(int contextId, int codeId)
{
    registry[contextId].insert(codeId);
}

void DependencyEvaluationEngine::unregDep(int contextId, int codeId)
{
    auto it = registry.find(contextId);
    if (it != registry.end())
        it->second.erase(codeId);
}

bool DependencyEvaluationEngine::verifyDep(int contextId, int codeId)
{
    auto it = registry.find(contextId);
    if (it == registry.end())
        return false;

    return it->second.find(codeId) != it->second.end();
}

void DependencyEvaluationEngine::regDep(int contextId, const std::vector<int>& codeIds)
{
    for (int codeId : codeIds)
    {
        regDep(contextId, codeId);
    }
}

bool DependencyEvaluationEngine::verifyDepAll(int contextId, const std::vector<int>& codeIds)
{
    for (int codeId : codeIds)
    {
        if (!verifyDep(contextId, codeId))
            return false;
    }
    return true;
}

bool DependencyEvaluationEngine::verifyDepAny(int contextId, const std::vector<int>& codeIds)
{
    for (int codeId : codeIds)
    {
        if (verifyDep(contextId, codeId))
            return true;
    }
    return false;
}

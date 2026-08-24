#ifndef DEE_H
#define DEE_H

#include <unordered_map>
#include <unordered_set>
#include <vector>

// Dependency Evaluation Engine

// Scope a dependency is registered/verified against.
enum ContextType {
    WORLD_CTX = 0,
    FACTION_CTX = 1,
    CITY_CTX = 2
};

int worldContext() ;
int factionContext(int factionId) ;
int cityContext(int cityId) ;

// @TODO: Add an internal double entry (contextId, codeId) map to keep track of the dependencies registered at each context.  This will allow for a more efficient verification of dependencies and will also allow for the unregistration of dependencies when a context is no longer needed.
class DependencyEvaluationEngine {
    private:
        std::unordered_map<int, std::unordered_set<int>> registry;

    public:
        DependencyEvaluationEngine();

        void regDep(int contextId, int codeId);

        void unregDep(int contextId, int codeId);

        bool verifyDep(int contextId, int codeId);

        bool verifyDepAll(int contextId, const std::vector<int>& codeIds);
        bool verifyDepAny(int contextId, const std::vector<int>& codeIds);

};

#endif // DEE_H

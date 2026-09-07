#ifndef BUILDABLE_H
#define BUILDABLE_H

#include <vector>
#include <unordered_map>

#include "resources.h"

enum BuildableType {
    UNIT = 0,
    BUILDING = 1
};

class Buildable {
    public:
    virtual BuildableType getType() = 0;
};

class BuildableFactory {
public:
    char name[256];
    virtual std::vector<int> getRequiredResources() = 0;
    virtual std::vector<Resource*> fullfillment(std::unordered_map<int, Resource*> availableResources) = 0;
    virtual Buildable* create() = 0;
    virtual std::vector<int> getDependencyCodes() {
        return dependencyCodes;
    }
protected:
    std::vector<int> dependencyCodes;
    void addDependencyCode(int codeId) {
        dependencyCodes.push_back(codeId);
    }
};





#endif // BUILDABLE_H
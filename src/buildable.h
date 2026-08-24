#ifndef BUILDABLE_H
#define BUILDABLE_H

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
    virtual int cost(int r_id) = 0;
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
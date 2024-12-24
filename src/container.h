#ifndef CONTAINER_H
#define CONTAINER_H

#define MAX 10000
#define MIN 1

#include <cassert>
#include <vector>
#include <unordered_map>

template <class T> class container
{
    protected:
        std::unordered_map<int, size_t> index;
        T data[MAX];
        size_t e_size;
        std::vector<int> prunning;

        size_t push_back(T value);
        size_t push_back(size_t initialregion, T value);
        size_t push_back(size_t initialregion, T value, int id);

    public:
        container();
        size_t push_back(T value, int id);
        size_t push_at_the_back(T value, int id);
        size_t assign(size_t pos, T value, int id);
        T operator[](size_t pos);
        T find(int id);
        size_t indexAt(int position);
        int indexOf(size_t pos);
        size_t first();
        size_t next(size_t pos);
        bool hasMore(size_t pos);
        size_t size();
        void erase(int id);
        bool isValid(size_t pos);
};

// This is a nasty solution to allow the linking of the template... Have a better one? Do it.
#include "container.cpp"

#endif // CONTAINER_H


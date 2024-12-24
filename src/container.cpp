#include "container.h"

#include <vector>
#include <mutex>
#include <unistd.h>
#include <unordered_map>

template <class T> container<T>::container()
{
    e_size = 0;

    for (size_t i = 0; i < MAX; i++)
    {
        data[i] = nullptr;
    }
}

template<class T> size_t container<T>::push_back(size_t initialregion, T value)
{
    size_t i;
    for(i=initialregion;i<MAX;i++)
    {
        if (data[i] == nullptr)
            break;

#ifdef DEBUG
        // @NOTE: Check if by mistake we are adding twice the same element to the container
        assert( data[i] != value || !"Adding the same element twice to the container!");
#endif
    }

    assert( i < MAX || !"The container is full and cannot hold more values.");

    data[i] = value;

    e_size++;

    return i;
}

template<class T> size_t container<T>::push_back(T value)
{
    return push_back(MIN, value);
}

template<class T> size_t container<T>::assign(size_t pos, T value, int id)
{
    assert( pos < MAX || !"The container is full and cannot hold more values.");

    if (data[pos] != nullptr)
    {
        // If there is an element there, erase it.
        erase(data[pos]->getGeom());
    }

    data[pos] = value;

    e_size++;    

    index[id] = pos;

    return pos;
}

template<class T> size_t container<T>::push_back(size_t initialregion, T value, int id)
{
    size_t pos = push_back(initialregion, value);
    index[id] = pos;
    return pos;
}

template<class T> size_t container<T>::push_back(T value, int id)
{
    return push_back(MIN,value,id);
}

template<class T> size_t container<T>::push_at_the_back(T value, int id)
{
    return push_back(e_size, value, id);
}

template<class T> T container<T>::operator[](size_t pos)
{
    T t;
    if (pos<MIN || pos>MAX)
        assert(!"This should not happen");
    if (data[pos] == nullptr)
        assert(!"Pointer is null");
    //printf("Accessing %d\n", index);
    t = data[pos];
    return t;
}

template<class T> T container<T>::find(int id)
{
    std::unordered_map<int, size_t>::const_iterator got = index.find (id);

    if ( got == index.end() )
    {
        return nullptr;
    }

    //std::cout << got->first << " is " << got->second;

    // @NOTE: If the key doesn't exists it gets created !!!  If you have to check before if it is present or not.
    size_t i = got->second; // geomidmap[geom];

    if (isValid(i))
    {
        return operator[](i);
    }
    else
    {
        return nullptr;
    }
}

template <class T> size_t container<T>::indexAt(int position)
{
    size_t i=MIN;
    int count=0;
    for(i=MIN;i<MAX;i++)
    {
        if (data[i] != nullptr)
            count++;

        if (count == position)
            break;
    }

    return i;
}

template <class T> int container<T>::indexOf(size_t element)
{
    size_t i=MIN;
    int count=0;
    for(i=MIN;i<MAX;i++)
    {
        if (data[i] != nullptr)
            count++;

        if (i == element)
            return count;
    }

    return 0;
}

template<class T> size_t container<T>::size()
{
    size_t i=MIN;
    size_t count=0;
    for(i=MIN;i<MAX;i++)
    {
        if (data[i] != nullptr)
            count++;
    }
    return count;
}

template<class T> bool container<T>::isValid(size_t pos)
{
    // @NOTE: Allow zero because I want to return FALSE for it.
    assert( pos>=0 && pos <MAX || !"Accessing an entity which is out of the valid range.");

    return (data[pos] != nullptr);
}

template<class T> bool container<T>::hasMore(size_t pos)
{
    size_t i=pos;
    while ( (i>=MIN && i<MAX) && data[i] == nullptr)
    {
        i++;
    }

    if (i>=MAX)
    {
        return false;
    }
    else
    {
        return true;
    }
}

template<class T> size_t container<T>::first()
{
    size_t i=MIN;
    while ( (i>=MIN && i<MAX) && data[i] == nullptr) i++;
    return i;
}

template<class T> size_t container<T>::next(size_t pos)
{
    size_t i=pos+1;
    while ( (i>=MIN && i<MAX) && data[i] == nullptr) i++;

    return i;

}
// The container is the ONLY component responsible of deleting all the entities.
template<class T> void container<T>::erase(int id)
{
    size_t pos = index[id];

    T e = data[pos];

    if (data[pos] != nullptr)
    {
        index.erase(id);

        data[pos] = nullptr;

        e_size--;

        delete e;
    }
}

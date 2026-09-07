#ifndef OBJECTS_H
#define OBJECTS_H

#include "data_types_worlds.hpp"

// voxel clusters types - each vector is {voxel local offset, size}, e.g. tree is 3 voxels above each other each with width and length 0.5 and height 1.0
inline ObjectDefinition tree = {
    {
        {0, 0, 0,	0.5f, 1.0f, 0.5f},
        {0, 1, 0,	0.5f, 1.0f, 0.5f},
        {0, 2, 0,	0.5f, 1.0f, 0.5f},
    }
};

inline ObjectDefinition brick = {
    {
        {0, 0, 0,	 1.0f, 2.0f, 1.0f},
    }
};

inline ObjectDefinition bush = {
    {
        {0, 0, 0,	 1.0f, 1.0f, 1.0f},
    }
};

inline ObjectDefinition building = {
    {
        {0, 0, 0,	 1.0f, 2.0f, 1.0f},
        {0, 1, 0,	 1.0f, 2.0f, 1.0f},
        {0, 2, 0,	 1.0f, 2.0f, 1.0f},
        {0, 3, 0,	 1.0f, 2.0f, 1.0f},
    }
};

inline ObjectDefinition rock = {
    {
        {0, 0, 0,	 1.5f, 1.5f, 1.5f},
    }
};

inline ObjectDefinition sand = {
    {
        {0, -0.5f, 0,	 1.0f, 0.5f, 1.0f},
    }
};

inline ObjectDefinition fence = {
    {
        {0, 0, 0,	 0.1f, 1.0f, 1.0f},
    }
};

inline ObjectDefinition puddle = {
    {
        {0, -0.9, 0,	 1.0f, 1.0f, 1.0f},
    }
};


#endif

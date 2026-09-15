#ifndef OBJECTS_H
#define OBJECTS_H

#include "data_types_worlds.hpp"
#include <string>
#include <unordered_map>

// voxel clusters types - each vector is {voxel local offset, size}, e.g. tree is 3 voxels above each other each with width and length 0.5 and height 1.0
// texture stays -1 (the default) everywhere here: objects keep the legacy behavior of taking their atlas tile from the world's hue_tile mapping, while worlds built against a shared multi-tile atlas give their objects an explicit texture instead
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

inline ObjectDefinition pillar = {
    {
        {0, 0, 0,	 0.4f, 3.0f, 0.4f},
    }
};

inline ObjectDefinition crate = {
    {
        {0, 0, 0,	 0.8f, 0.8f, 0.8f},
    }
};

// look an object up by the name the registry file uses - the single name -> definition mapping, so the registry and the world building path cannot drift apart
inline const ObjectDefinition *object_by_name(const char *name) {
	static const std::unordered_map<std::string, const ObjectDefinition*> library = {
		{"tree", &tree},
		{"brick", &brick},
		{"bush", &bush},
		{"building", &building},
		{"rock", &rock},
		{"sand", &sand},
		{"fence", &fence},
		{"puddle", &puddle},
		{"pillar", &pillar},
		{"crate", &crate},
	};
	if (name == nullptr) return nullptr;
	auto it = library.find(name);
	return (it == library.end()) ? nullptr : it->second;
}

#endif

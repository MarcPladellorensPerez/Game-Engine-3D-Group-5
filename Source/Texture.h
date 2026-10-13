#pragma once
#include <string>

struct Texture {
    unsigned int id;
    std::string type;
    std::string path;

    static unsigned int TextureFromFile(const char* path, const std::string& directory);
};

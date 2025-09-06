#ifndef SERIALISATION_H
#define SERIALISATION_H

#include "PreCompile.h"
#include <document.h>
#include <writer.h>
#include <stringbuffer.h>

struct Config {
    int windowWidth;
    int windowHeight;
    bool fullscreen;
    std::string title;
};

std::string SerializeConfig(const Config& config);
Config DeserializeConfig(const std::string& jsonStr);

#endif // SERIALISATION_H

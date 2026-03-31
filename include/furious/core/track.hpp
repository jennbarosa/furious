#pragma once

#include <string>

namespace furious {

struct Track {
    std::string name;
    bool visible = true;
    bool locked = false;
    bool muted = false;
    bool soloed = false;
};

} // namespace furious

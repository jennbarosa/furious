#pragma once

#include <string>

namespace furious {

struct Track {
    std::string name;
    bool visible = true;
    bool locked = false;
};

} // namespace furious

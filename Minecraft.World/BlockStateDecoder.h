#pragma once
#include <string>
#include "Tile.h"

namespace BlockStateDecoder {

// door stuff is here cause idk what i was doing when i intially started this
struct DoorProps {
    int dir;
    std::wstring dirName;
    bool open;
    bool upper;
    bool hingeRight;
};

DoorProps decodeDoor(int composite);
std::wstring doorPropsToString(const DoorProps &p);

}

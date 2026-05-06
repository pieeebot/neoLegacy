#pragma once
#include <string>
#include <functional>

namespace BlockStateDecoderRegistry 
{
    using DecoderFn = std::function<std::wstring(int)>;

    void registerDecoder(int tileId, DecoderFn fn);
    
    std::wstring decode(int tileId, int composite); 
}

#include "BlockStateDecoderRegistry.h"
#include <unordered_map>
#include <mutex>

namespace BlockStateDecoderRegistry {

static std::unordered_map<int, DecoderFn> *g_map = nullptr;
static std::mutex g_mapMutex;

static void ensureMap()
{
    if (!g_map) g_map = new std::unordered_map<int, DecoderFn>();
}

void registerDecoder(int tileId, DecoderFn fn)
{
    std::lock_guard<std::mutex> l(g_mapMutex);
    ensureMap();
    (*g_map)[tileId] = fn;
}

std::wstring decode(int tileId, int composite)
{
    std::lock_guard<std::mutex> l(g_mapMutex);
    ensureMap();
    auto it = g_map->find(tileId);
    if (it == g_map->end()) return L"";
    try {
        return it->second(composite);
    } catch (...) {
        return L"";
    }
}

}

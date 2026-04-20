#include "DataRefManager.h"
#include "Logger.h"

namespace a320 {

// ── Helpers ────────────────────────────────────────────────────────────────

XPLMDataRef DataRefManager::find(const char* name)
{
    auto it = m_cache.find(name);
    if (it != m_cache.end()) return it->second;

    XPLMDataRef ref = XPLMFindDataRef(name);
    if (!ref) LOG_WARN("DataRef not found: {}", name);
    m_cache[name] = ref;
    return ref;
}

// ── Sim dataref reads ──────────────────────────────────────────────────────

int DataRefManager::getInt(const char* name)
{
    XPLMDataRef r = find(name);
    return r ? XPLMGetDatai(r) : 0;
}

float DataRefManager::getFloat(const char* name)
{
    XPLMDataRef r = find(name);
    return r ? XPLMGetDataf(r) : 0.f;
}

double DataRefManager::getDouble(const char* name)
{
    XPLMDataRef r = find(name);
    return r ? XPLMGetDatad(r) : 0.0;
}

int DataRefManager::getFloatArray(const char* name, float* out, int offset, int count)
{
    XPLMDataRef r = find(name);
    return r ? XPLMGetDatavf(r, out, offset, count) : 0;
}

int DataRefManager::getIntArray(const char* name, int* out, int offset, int count)
{
    XPLMDataRef r = find(name);
    return r ? XPLMGetDatavi(r, out, offset, count) : 0;
}

// ── Sim dataref writes ─────────────────────────────────────────────────────

void DataRefManager::setInt(const char* name, int v)
{
    XPLMDataRef r = find(name);
    if (r) XPLMSetDatai(r, v);
}

void DataRefManager::setFloat(const char* name, float v)
{
    XPLMDataRef r = find(name);
    if (r) XPLMSetDataf(r, v);
}

void DataRefManager::setDouble(const char* name, double v)
{
    XPLMDataRef r = find(name);
    if (r) XPLMSetDatad(r, v);
}

void DataRefManager::setFloatArray(const char* name, float* in, int offset, int count)
{
    XPLMDataRef r = find(name);
    if (r) XPLMSetDatavf(r, in, offset, count);
}

void DataRefManager::setIntArray(const char* name, int* in, int offset, int count)
{
    XPLMDataRef r = find(name);
    if (r) XPLMSetDatavi(r, in, offset, count);
}

// ── Custom dataref registration ────────────────────────────────────────────

float DataRefManager::cbGetFloat(void* refcon)
{
    return static_cast<CustomFloat*>(refcon)->value;
}
void DataRefManager::cbSetFloat(void* refcon, float v)
{
    static_cast<CustomFloat*>(refcon)->value = v;
}
int DataRefManager::cbGetInt(void* refcon)
{
    return static_cast<CustomInt*>(refcon)->value;
}
void DataRefManager::cbSetInt(void* refcon, int v)
{
    static_cast<CustomInt*>(refcon)->value = v;
}

void DataRefManager::registerFloat(const char* name, float initial, bool writable)
{
    if (m_customFloats.count(name)) return;
    auto& entry = m_customFloats[name];
    entry.value = initial;
    entry.ref = XPLMRegisterDataAccessor(
        name, xplmType_Float, writable ? 1 : 0,
        nullptr, nullptr,
        cbGetFloat, writable ? cbSetFloat : nullptr,
        nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        &entry, &entry);
}

void DataRefManager::registerInt(const char* name, int initial, bool writable)
{
    if (m_customInts.count(name)) return;
    auto& entry = m_customInts[name];
    entry.value = initial;
    entry.ref = XPLMRegisterDataAccessor(
        name, xplmType_Int, writable ? 1 : 0,
        cbGetInt, writable ? cbSetInt : nullptr,
        nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        &entry, &entry);
}

void DataRefManager::setCustomFloat(const char* name, float v)
{
    auto it = m_customFloats.find(name);
    if (it != m_customFloats.end()) it->second.value = v;
}

void DataRefManager::setCustomInt(const char* name, int v)
{
    auto it = m_customInts.find(name);
    if (it != m_customInts.end()) it->second.value = v;
}

float DataRefManager::getCustomFloat(const char* name) const
{
    auto it = m_customFloats.find(name);
    return it != m_customFloats.end() ? it->second.value : 0.f;
}

int DataRefManager::getCustomInt(const char* name) const
{
    auto it = m_customInts.find(name);
    return it != m_customInts.end() ? it->second.value : 0;
}

void DataRefManager::unregisterAll()
{
    for (auto& [name, entry] : m_customFloats)
        XPLMUnregisterDataAccessor(entry.ref);
    for (auto& [name, entry] : m_customInts)
        XPLMUnregisterDataAccessor(entry.ref);
    m_customFloats.clear();
    m_customInts.clear();
    m_cache.clear();
}

} // namespace a320

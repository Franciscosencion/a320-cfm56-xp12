#include "DataRefManager.h"
#include "Logger.h"
#ifndef UNIT_TESTING
#  include <XPLM/XPLMDataAccess.h>
#endif

namespace a320 {

// ── Helpers ────────────────────────────────────────────────────────────────

XPLMDataRef DataRefManager::find(const char* name)
{
#ifndef UNIT_TESTING
    auto it = m_cache.find(name);
    if (it != m_cache.end()) return it->second;
    XPLMDataRef ref = XPLMFindDataRef(name);
    if (!ref) LOG_WARN("DataRef not found: {}", name);
    m_cache[name] = ref;
    return ref;
#else
    (void)name;
    return nullptr;
#endif
}

// ── Sim dataref reads ──────────────────────────────────────────────────────

int DataRefManager::getInt(const char* name)
{
#ifndef UNIT_TESTING
    XPLMDataRef r = find(name);
    return r ? XPLMGetDatai(r) : 0;
#else
    (void)name; return 0;
#endif
}

float DataRefManager::getFloat(const char* name)
{
#ifndef UNIT_TESTING
    XPLMDataRef r = find(name);
    return r ? XPLMGetDataf(r) : 0.f;
#else
    (void)name; return 0.f;
#endif
}

double DataRefManager::getDouble(const char* name)
{
#ifndef UNIT_TESTING
    XPLMDataRef r = find(name);
    return r ? XPLMGetDatad(r) : 0.0;
#else
    (void)name; return 0.0;
#endif
}

int DataRefManager::getFloatArray(const char* name, float* out, int offset, int count)
{
#ifndef UNIT_TESTING
    XPLMDataRef r = find(name);
    return r ? XPLMGetDatavf(r, out, offset, count) : 0;
#else
    (void)name; (void)out; (void)offset; (void)count; return 0;
#endif
}

int DataRefManager::getIntArray(const char* name, int* out, int offset, int count)
{
#ifndef UNIT_TESTING
    XPLMDataRef r = find(name);
    return r ? XPLMGetDatavi(r, out, offset, count) : 0;
#else
    (void)name; (void)out; (void)offset; (void)count; return 0;
#endif
}

// ── Sim dataref writes ─────────────────────────────────────────────────────

void DataRefManager::setInt(const char* name, int v)
{
#ifndef UNIT_TESTING
    XPLMDataRef r = find(name);
    if (r) XPLMSetDatai(r, v);
#else
    (void)name; (void)v;
#endif
}

void DataRefManager::setFloat(const char* name, float v)
{
#ifndef UNIT_TESTING
    XPLMDataRef r = find(name);
    if (r) XPLMSetDataf(r, v);
#else
    (void)name; (void)v;
#endif
}

void DataRefManager::setDouble(const char* name, double v)
{
#ifndef UNIT_TESTING
    XPLMDataRef r = find(name);
    if (r) XPLMSetDatad(r, v);
#else
    (void)name; (void)v;
#endif
}

void DataRefManager::setFloatArray(const char* name, float* in, int offset, int count)
{
#ifndef UNIT_TESTING
    XPLMDataRef r = find(name);
    if (r) XPLMSetDatavf(r, in, offset, count);
#else
    (void)name; (void)in; (void)offset; (void)count;
#endif
}

void DataRefManager::setIntArray(const char* name, int* in, int offset, int count)
{
#ifndef UNIT_TESTING
    XPLMDataRef r = find(name);
    if (r) XPLMSetDatavi(r, in, offset, count);
#else
    (void)name; (void)in; (void)offset; (void)count;
#endif
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
#ifndef UNIT_TESTING
    entry.ref = XPLMRegisterDataAccessor(
        name, xplmType_Float, writable ? 1 : 0,
        nullptr, nullptr,
        cbGetFloat, writable ? cbSetFloat : nullptr,
        nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        &entry, &entry);
#else
    (void)writable;
    entry.ref = nullptr;
#endif
}

void DataRefManager::registerInt(const char* name, int initial, bool writable)
{
    if (m_customInts.count(name)) return;
    auto& entry = m_customInts[name];
    entry.value = initial;
#ifndef UNIT_TESTING
    entry.ref = XPLMRegisterDataAccessor(
        name, xplmType_Int, writable ? 1 : 0,
        cbGetInt, writable ? cbSetInt : nullptr,
        nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        &entry, &entry);
#else
    (void)writable;
    entry.ref = nullptr;
#endif
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
#ifndef UNIT_TESTING
    for (auto& [name, entry] : m_customFloats)
        XPLMUnregisterDataAccessor(entry.ref);
    for (auto& [name, entry] : m_customInts)
        XPLMUnregisterDataAccessor(entry.ref);
#endif
    m_customFloats.clear();
    m_customInts.clear();
    m_cache.clear();
}

} // namespace a320

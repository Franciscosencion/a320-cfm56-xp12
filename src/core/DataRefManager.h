#pragma once
#ifndef UNIT_TESTING
#  include <XPLM/XPLMDataAccess.h>
#  include <XPLM/XPLMDefs.h>
#else
   using XPLMDataRef = void*;
#endif
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace a320 {

// Wraps all XPLM dataref access. Every system reads/writes through here.
// Provides:
//   - Lazy lookup + cache of XPLMDataRef handles
//   - Typed read/write helpers
//   - Registration of custom (outbound) datarefs backed by local storage
class DataRefManager {
public:
    // ── Read built-in sim datarefs ─────────────────────────────────────────
    int    getInt   (const char* name);
    float  getFloat (const char* name);
    double getDouble(const char* name);
    int    getFloatArray(const char* name, float* out, int offset, int count);
    int    getIntArray  (const char* name, int*   out, int offset, int count);

    // ── Write built-in sim datarefs ────────────────────────────────────────
    void setInt   (const char* name, int    v);
    void setFloat (const char* name, float  v);
    void setDouble(const char* name, double v);
    void setFloatArray(const char* name, float* in, int offset, int count);
    void setIntArray  (const char* name, int*   in, int offset, int count);

    // ── Register custom (plugin-owned) datarefs ────────────────────────────
    // Backed by a float value stored inside the manager.
    // External display software reads these over the network; other X-Plane
    // plugins can also subscribe via standard XPLM calls.
    void registerFloat(const char* name, float initialValue = 0.f, bool writable = false);
    void registerInt  (const char* name, int   initialValue = 0,   bool writable = false);

    void setCustomFloat(const char* name, float v);
    void setCustomInt  (const char* name, int   v);
    float getCustomFloat(const char* name) const;
    int   getCustomInt  (const char* name) const;

    void unregisterAll();

    static DataRefManager& instance() {
        static DataRefManager mgr;
        return mgr;
    }

private:
    XPLMDataRef find(const char* name);

    std::unordered_map<std::string, XPLMDataRef> m_cache;

    struct CustomFloat { XPLMDataRef ref; float value; };
    struct CustomInt   { XPLMDataRef ref; int   value; };
    std::unordered_map<std::string, CustomFloat> m_customFloats;
    std::unordered_map<std::string, CustomInt>   m_customInts;

    // XPLM callbacks for custom datarefs
    static float  cbGetFloat(void* refcon);
    static void   cbSetFloat(void* refcon, float v);
    static int    cbGetInt  (void* refcon);
    static void   cbSetInt  (void* refcon, int   v);
};

} // namespace a320

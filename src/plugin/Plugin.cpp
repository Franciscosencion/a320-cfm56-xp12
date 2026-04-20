#include "Plugin.h"
#include "core/FlightLoop.h"
#include "core/Logger.h"
#include "core/Config.h"
#include "core/DataRefManager.h"
#include "network/NetworkServer.h"
#include "systems/electrical/ElectricalSystem.h"
#include "systems/pneumatics/PneumaticSystem.h"
#include "systems/ecam/SDPages.h"

#include <XPLM/XPLMDefs.h>
#include <XPLM/XPLMProcessing.h>
#include <XPLM/XPLMDataAccess.h>

#include <cstring>
#include <memory>

static std::unique_ptr<a320::FlightLoop>        g_flightLoop;
static std::unique_ptr<a320::NetworkServer>     g_networkServer;
static std::unique_ptr<a320::ElectricalSystem>  g_electrical;
static std::unique_ptr<a320::PneumaticSystem>   g_pneumatic;
static std::unique_ptr<a320::SDPages>           g_sdPages;
static uint32_t                                  g_seqNum = 0;

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    std::strcpy(outName, "A320 CFM56");
    std::strcpy(outSig,  "a320.cfm56.xp12");
    std::strcpy(outDesc, "Airbus A320 CFM56 home-cockpit systems plugin for X-Plane 12");

    a320::Logger::init("a320_plugin.log");
    LOG_INFO("A320 plugin starting (v{}.{}.{})", 0, 1, 0);

    auto cfg = a320::Config::loadOrDefault("a320/config/systems_config.json");
    if (!cfg) {
        LOG_WARN("systems_config.json not found — using defaults");
    }

    return 1;
}

PLUGIN_API void XPluginStop(void)
{
    g_networkServer.reset();
    g_flightLoop.reset();
    LOG_INFO("A320 plugin stopped");
    a320::Logger::shutdown();
}

PLUGIN_API int XPluginEnable(void)
{
    LOG_INFO("A320 plugin enabled");

    g_networkServer = std::make_unique<a320::NetworkServer>(45010, 45011);
    g_networkServer->start();

    g_electrical = std::make_unique<a320::ElectricalSystem>();
    g_electrical->init();

    g_pneumatic = std::make_unique<a320::PneumaticSystem>();
    g_pneumatic->init();

    g_sdPages = std::make_unique<a320::SDPages>(*g_networkServer);

    g_flightLoop = std::make_unique<a320::FlightLoop>();

    // Cache frequently-read sim datarefs
    static XPLMDataRef drAltFt    = XPLMFindDataRef("sim/flightmodel/position/elevation");
    static XPLMDataRef drPressHpa = XPLMFindDataRef("sim/weather/barometer_sealevel_inhg");
    static XPLMDataRef drOatC     = XPLMFindDataRef("sim/weather/temperature_ambient_c");
    static XPLMDataRef drOnGround = XPLMFindDataRef("sim/flightmodel2/misc/on_ground");
    static XPLMDataRef drN1_1     = XPLMFindDataRef("sim/cockpit2/engine/indicators/N2_percent_engine");

    // Register 50 Hz systems tick
    g_flightLoop->addSystem([](float dt) {
        // Feed sim state into systems
        float altM     = drAltFt    ? XPLMGetDataf(drAltFt)    : 0.f;
        float altFt    = altM * 3.28084f;
        float pressHpa = drPressHpa ? XPLMGetDataf(drPressHpa) * 33.8639f : 1013.25f;
        float oatC     = drOatC     ? XPLMGetDataf(drOatC)     : 15.f;
        bool  onGround = drOnGround ? XPLMGetDatai(drOnGround) != 0 : true;

        float n2[2] = {0.f, 0.f};
        if (drN1_1) XPLMGetDatavf(drN1_1, n2, 0, 2);

        // Electrical
        g_electrical->setEng1N2(n2[0]);
        g_electrical->setEng2N2(n2[1]);
        g_electrical->update(dt);

        // Pneumatic
        g_pneumatic->setEng1N2(n2[0]);
        g_pneumatic->setEng2N2(n2[1]);
        g_pneumatic->setAircraftAltFt(altFt);
        g_pneumatic->setAmbientPressHpa(pressHpa);
        g_pneumatic->setOatDegC(oatC);
        g_pneumatic->setOnGround(onGround);
        g_pneumatic->update(dt);

        uint32_t simTimeMs = static_cast<uint32_t>(XPLMGetElapsedTime() * 1000.f);
        g_sdPages->sendELEC (*g_electrical, simTimeMs, g_seqNum);
        g_sdPages->sendBLEED(*g_pneumatic,  simTimeMs, g_seqNum);
        g_sdPages->sendPRESS(*g_pneumatic,  simTimeMs, g_seqNum);
        g_sdPages->sendCOND (*g_pneumatic,  simTimeMs, g_seqNum);
    });

    g_flightLoop->registerCallbacks();

    return 1;
}

PLUGIN_API void XPluginDisable(void)
{
    LOG_INFO("A320 plugin disabled");
    if (g_flightLoop)    g_flightLoop->unregisterCallbacks();
    if (g_networkServer) g_networkServer->stop();
    g_sdPages.reset();
    g_pneumatic.reset();
    g_electrical.reset();
    a320::DataRefManager::instance().unregisterAll();
}

PLUGIN_API void XPluginReceiveMessage(int /*inFrom*/, int /*inMsg*/, void* /*inParam*/)
{
    // Future: handle XPLM_MSG_PLANE_LOADED, XPLM_MSG_AIRPORT_LOADED, etc.
}

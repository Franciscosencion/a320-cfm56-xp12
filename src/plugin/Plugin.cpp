#include "Plugin.h"
#include "core/FlightLoop.h"
#include "core/Logger.h"
#include "core/Config.h"
#include "network/NetworkServer.h"

#include <XPLM/XPLMDefs.h>

#include <cstring>
#include <memory>

static std::unique_ptr<a320::FlightLoop>     g_flightLoop;
static std::unique_ptr<a320::NetworkServer>  g_networkServer;

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

    g_flightLoop = std::make_unique<a320::FlightLoop>();
    g_flightLoop->registerCallbacks();

    return 1;
}

PLUGIN_API void XPluginDisable(void)
{
    LOG_INFO("A320 plugin disabled");
    if (g_flightLoop)    g_flightLoop->unregisterCallbacks();
    if (g_networkServer) g_networkServer->stop();
}

PLUGIN_API void XPluginReceiveMessage(int /*inFrom*/, int /*inMsg*/, void* /*inParam*/)
{
    // Future: handle XPLM_MSG_PLANE_LOADED, XPLM_MSG_AIRPORT_LOADED, etc.
}

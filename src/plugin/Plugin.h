#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// X-Plane plugin entry points
int  XPluginStart(char* outName, char* outSig, char* outDesc);
void XPluginStop(void);
int  XPluginEnable(void);
void XPluginDisable(void);
void XPluginReceiveMessage(int inFrom, int inMsg, void* inParam);

#ifdef __cplusplus
}
#endif

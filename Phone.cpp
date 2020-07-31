//---------------------------------------------------------------------------

#define _EXPORTING
#include "..\tSIP\tSIP\phone\Phone.h"
#include "..\tSIP\tSIP\phone\PhoneSettings.h"
#include "..\tSIP\tSIP\phone\PhoneCapabilities.h"
#include "Log.h"
#include <assert.h>
#include <algorithm>	// needed by Utils::in_group
#include "Utils.h"
#include <string>
#include <fstream>
#include <json/json.h>

//---------------------------------------------------------------------------

static const struct S_PHONE_DLL_INTERFACE dll_interface =
{DLL_INTERFACE_MAJOR_VERSION, DLL_INTERFACE_MINOR_VERSION};

// callback ptrs
CALLBACK_LOG lpLogFn = NULL;
CALLBACK_CONNECT lpConnectFn = NULL;
CALLBACK_KEY lpKeyFn = NULL;
CALLBACK_SET_VARIABLE lpSetVariableFn = NULL;

void *callbackCookie;	///< used by upper class to distinguish library instances when receiving callbacks

#define DEFAULT_CMD "START CMD /C \"ECHO Add command to be executed when ringing to plugin config file (after closing softphone) && PAUSE\""

namespace {
std::string ringCmd = DEFAULT_CMD;
}


extern "C" __declspec(dllexport) void GetPhoneInterfaceDescription(struct S_PHONE_DLL_INTERFACE* interf) {
    interf->majorVersion = dll_interface.majorVersion;
    interf->minorVersion = dll_interface.minorVersion;
}

void Log(const char* txt) {
    if (lpLogFn)
        lpLogFn(callbackCookie, txt);
}

void Connect(int state, char *szMsgText) {
    if (lpConnectFn)
        lpConnectFn(callbackCookie, state, szMsgText);
}

void Key(int keyCode, int state) {
    if (lpKeyFn)
        lpKeyFn(callbackCookie, keyCode, state);
}

void SetCallbacks(void *cookie, CALLBACK_LOG lpLog, CALLBACK_CONNECT lpConnect, CALLBACK_KEY lpKey) {
    assert(cookie && lpLog && lpConnect && lpKey);
    lpLogFn = lpLog;
    lpConnectFn = lpConnect;
    lpKeyFn = lpKey;
    callbackCookie = cookie;
    lpLogFn(callbackCookie, "GetIdleTime dll loaded\n");

    //armScope.callbackLog = Log;
    //armScope.callbackConnect = Connect;
}

void GetPhoneCapabilities(struct S_PHONE_CAPABILITIES **caps) {
    static struct S_PHONE_CAPABILITIES capabilities = {
        0
    };
    *caps = &capabilities;
}

void ShowSettings(HANDLE parent) {
    MessageBox((HWND)parent, "No additional settings.", "Device DLL", MB_ICONINFORMATION);
}

static int WorkerThreadStart(void);
static int WorkerThreadStop(void);

int Connect(void) {
    Log("Connect\n");
    WorkerThreadStart();
    return 0;
}

int Disconnect(void) {
    Log("Disconnect\n");
    WorkerThreadStop();
    return 0;
}

static bool bSettingsReaded = false;

static int GetDefaultSettings(struct S_PHONE_SETTINGS* settings) {
    settings->ring = 1;

    bSettingsReaded = true;
    return 0;
}

int GetPhoneSettings(struct S_PHONE_SETTINGS* settings) {
    assert(settings);
    return GetDefaultSettings(settings);
}

int SavePhoneSettings(struct S_PHONE_SETTINGS* settings) {
    return 0;
}

int SetRegistrationState(int state) {
    return 0;
}

int SetCallState(int state, const char* display) {
    return 0;
}

int Ring(int state) {
    //MessageBox(NULL, "Ring", "Device DLL", MB_ICONINFORMATION);
    return 0;
}

void SetSetVariableCallback(CALLBACK_SET_VARIABLE lpFn) {
	lpSetVariableFn = lpFn;
}

bool connected = false;
bool exited = true;

DWORD WINAPI WorkerThreadProc(LPVOID data) {
    while (connected) {
        LASTINPUTINFO info;
        info.cbSize = sizeof(info);
        if (GetLastInputInfo(&info) != 0) {
            int time = (GetTickCount() - info.dwTime) / 1000;
            char timeStr[32];
            itoa(time, timeStr, 10);
            lpSetVariableFn(callbackCookie, "InputIdleTime", timeStr);
        } else {
            Log("GetIdleTime DLL: GetLastInputInfo failed\n");
        }
        for (int i=0; i<10; i++) {
            Sleep(100);
            if (exited)
                break;
        }
    }

    exited = true;
    return 0;
}


int WorkerThreadStart(void) {
    DWORD dwtid;
    exited = false;
    connected = true;
    HANDLE Thread = CreateThread(NULL, 0, WorkerThreadProc, /*this*/NULL, 0, &dwtid);
    if (Thread == NULL) {
        Log("Failed to create worker thread.");
        connected = false;
        exited = true;
    } else {
        Log("Worker thread created.\n");
    }
    return 0;
}

int WorkerThreadStop(void) {
    connected = false;
    while (!exited) {
        Sleep(50);
    }
    return 0;
}

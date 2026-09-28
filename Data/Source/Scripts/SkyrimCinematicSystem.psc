Scriptname SkyrimCinematicSystem extends Quest

function FindFiles() global native
int function GetFileCount() global native
string function GetFilename(int index) global native

bool function StartScript(string filename, bool hideInterface = false) global native
function StopScript() global native
bool function IsScriptRunning() global native

bool function RegisterForCinematicScriptEvents(Form listener) global native
bool function UnregisterForCinematicScriptEvents(Form listener) global native

bool function StartRecording() global native
bool function StopRecording() global native
bool function IsScriptRecording() global native
bool function SaveRecordingPoint(bool absolute) global native

float function GetCurrentPosX() global native
float function GetCurrentPosY() global native
float function GetCurrentPosZ() global native
float function GetCurrentRotX() global native
float function GetCurrentRotZ() global native
float function GetCurrentFOV() global native
float function GetCurrentTimeMultiplier() global native


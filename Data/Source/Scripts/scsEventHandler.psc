Scriptname scsEventHandler extends ReferenceAlias

scsConfigMenu Property MCM Auto

event OnScriptFinishedEvent(string cameraScriptName, bool cancelled)
    if(cancelled)
        Debug.Notification(cameraScriptName + " camera script has been cancelled.")
    else
        Debug.Notification(cameraScriptName + " camera script has finished.")
    endif
endevent

Event OnInit()
    RegisterForModEvent("scs_KeyMapChanged", "OnKeyMapChanged")
    UpdateKeyRegistrations()
EndEvent

Event OnPlayerLoadGame()
    UpdateKeyRegistrations()
EndEvent

Event OnKeyMapChanged(String eventName, String strArg, Float numArg, Form sender)
    UpdateKeyRegistrations()
EndEvent

Function UpdateKeyRegistrations()
    UnregisterForAllKeys()
    If (MCM == None)
        Debug.Trace("SkyrimCinematicSystem: MCM property is not set; no hotkeys registered.")
        Return
    EndIf

    MCM.EnsureKeyArray()
    If (MCM.iKeys == None)
        Debug.Trace("SkyrimCinematicSystem: MCM key array is unavailable; no hotkeys registered.")
        Return
    EndIf

    Int index = 0
    While (index < MCM.iKeys.Length)
        If (MCM.iKeys[index] > 0)
            RegisterForKey(MCM.iKeys[index])
        EndIf
        index += 1
    EndWhile
    Debug.Trace("SkyrimCinematicSystem: hotkey registrations refreshed.")
EndFunction

Event OnKeyDown(Int keyCode)
    If (Utility.IsInMenuMode())
        Return
    EndIf
    If (MCM == None)
        Debug.Trace("SkyrimCinematicSystem: key event received without an MCM instance.")
        Return
    EndIf
    MCM.EnsureKeyArray()
    If (MCM.iKeys == None || MCM.iKeys.Length < MCM.iKeyIndexMax)
        Debug.Trace("SkyrimCinematicSystem: key event received without a valid MCM key array.")
        Return
    EndIf

    If (keyCode == MCM.iKeys[MCM.START_STOP_SCRIPT])
        If (SkyrimCinematicSystem.IsScriptRunning())
            SkyrimCinematicSystem.StopScript()
        Else
            SkyrimCinematicSystem.StartScript(MCM.SelectedScript, MCM.HideInterface)
        EndIf

    ElseIf (keyCode == MCM.iKeys[MCM.PREVIOUS_SCRIPT])
        MCM.SelectPreviousScript()

    ElseIf (keyCode == MCM.iKeys[MCM.NEXT_SCRIPT])
        MCM.SelectNextScript()

    ElseIf (keyCode == MCM.iKeys[MCM.START_STOP_RECORDING])
        If (SkyrimCinematicSystem.IsScriptRecording())
            SkyrimCinematicSystem.StopRecording()
        Else
            SkyrimCinematicSystem.StartRecording()
        EndIf

    ElseIf (keyCode == MCM.iKeys[MCM.RELATIVE_POINT_RECORDING])
        If (SkyrimCinematicSystem.IsScriptRecording())
            SkyrimCinematicSystem.SaveRecordingPoint(False)
        EndIf

    ElseIf (keyCode == MCM.iKeys[MCM.ABSOLUTE_POINT_RECORDING])
        If (SkyrimCinematicSystem.IsScriptRecording())
            SkyrimCinematicSystem.SaveRecordingPoint(True)
        EndIf
    EndIf
EndEvent
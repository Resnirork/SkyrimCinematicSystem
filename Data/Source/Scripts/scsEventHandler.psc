Scriptname scsEventHandler extends ReferenceAlias

scsConfigMenu Property MCM Auto

Event OnInit()
    RegisterCinematicEventListener()
    RegisterForModEvent("scs_KeyMapChanged", "OnKeyMapChanged")
    UpdateKeyRegistrations()
EndEvent

Event OnPlayerLoadGame()
    RegisterCinematicEventListener()
    UpdateKeyRegistrations()
EndEvent

Event OnKeyMapChanged()
    UpdateKeyRegistrations()
EndEvent

Function RegisterCinematicEventListener()
    Bool newlyRegistered = SkyrimCinematicSystem.RegisterForCinematicScriptEvents(self.GetReference())
    Debug.Trace("scsEventHandler: Event registration native called; newly registered: " + newlyRegistered)
EndFunction

Event OnScriptFinishedEvent(String cameraScriptName, Bool cancelled)
    Debug.Trace("scsConfigMenu: Received OnScriptFinishedEvent for camera script: " + cameraScriptName + ", cancelled: " + cancelled)
    If (cancelled)
        Debug.Notification(cameraScriptName + " camera script has been cancelled.")
    Else
        Debug.Notification(cameraScriptName + " camera script has finished.")
    EndIf
EndEvent

Function UpdateKeyRegistrations()
    UnregisterForAllKeys()
    MCM.EnsureKeyArray()

    Int index = 0
    While (index < MCM.iKeys.Length)
        If (MCM.iKeys[index] > 0)
            RegisterForKey(MCM.iKeys[index])
        EndIf
        index += 1
    EndWhile
    Debug.Trace("scsEventHandler: Updated key registrations for " + MCM.iKeys.Length + " keys.")
EndFunction

Event OnKeyDown(Int keyCode)
    If (Utility.IsInMenuMode())
        Return
    EndIf

    Debug.Trace("scsEventHandler: Key pressed with keyCode " + keyCode)

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
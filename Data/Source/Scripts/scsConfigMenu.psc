Scriptname scsConfigMenu extends SKI_ConfigBase

Int Function GetVersion()
    return 3
EndFunction

Int[] Property iKeys Auto

Int Property START_STOP_SCRIPT = 0 AutoReadonly
Int Property PREVIOUS_SCRIPT = 1 AutoReadonly
Int Property NEXT_SCRIPT = 2 AutoReadonly
Int Property START_STOP_RECORDING = 3 AutoReadonly
Int Property RELATIVE_POINT_RECORDING = 4 AutoReadonly
Int Property ABSOLUTE_POINT_RECORDING = 5 AutoReadonly
Int Property iKeyIndexMax = 6 AutoReadonly

String[] scriptFiles
Int selectedScriptIndex = 0
String Property SelectedScript Auto
Bool Property HideInterface Auto

Event OnConfigInit()
    Pages = new String[2]
    Pages[0] = "$scs_Page_PLAYBACK"
    Pages[1] = "$scs_Page_KEYBINDINGS"
    EnsureKeyArray()
    RefreshScriptFiles()
    NotifyKeyMapChanged()
EndEvent

Event OnVersionUpdate(Int version)
    OnConfigInit()
EndEvent

Function EnsureKeyArray()
    If (!iKeys)
        iKeys = Utility.CreateIntArray(iKeyIndexMax)
        Debug.Trace("scsConfigMenu: Created iKeys array with size " + iKeyIndexMax)
    ElseIf (iKeys.Length < iKeyIndexMax)
        iKeys = Utility.ResizeIntArray(iKeys, iKeyIndexMax)
        Debug.Trace("scsConfigMenu: Resized iKeys array to size " + iKeyIndexMax)
    EndIf
EndFunction

Function ClearAllKeyBindings()
    Int index = 0
    While (index < iKeyIndexMax)
        iKeys[index] = 0
        index += 1
    EndWhile
    NotifyKeyMapChanged()
    Debug.Trace("scsConfigMenu: Cleared all key bindings")
EndFunction

Function RefreshScriptFiles()
    SkyrimCinematicSystem.FindFiles()
    Int fileCount = SkyrimCinematicSystem.GetFileCount()
    If (fileCount > 128)
        fileCount = 128
    EndIf

    scriptFiles = Utility.CreateStringArray(fileCount)
    Int index = 0
    While (index < fileCount)
        scriptFiles[index] = SkyrimCinematicSystem.GetFilename(index)
        index += 1
    EndWhile

    If (fileCount == 0)
        SelectedScript = ""
        selectedScriptIndex = 0
        Return
    EndIf

    index = 0
    While (index < fileCount)
        If (scriptFiles[index] == SelectedScript)
            selectedScriptIndex = index
            Return
        EndIf
        index += 1
    EndWhile

    selectedScriptIndex = 0
    SelectedScript = scriptFiles[0]
EndFunction

Function NotifyKeyMapChanged()
    Int eventHandle = ModEvent.Create("scs_KeyMapChanged")
    If (eventHandle)
        ModEvent.Send(eventHandle)
    EndIf
EndFunction

Event OnPageReset(String page)
    String displayName = SelectedScript

    EnsureKeyArray()
    If (!scriptFiles)
        RefreshScriptFiles()
    EndIf

    If (page == "" || Page == Pages[0])
        Int scriptFlags = OPTION_FLAG_NONE
        AddHeaderOption("$scs_Header_SCRIPT_SELECT")
        AddEmptyOption()
        If (!scriptFiles || scriptFiles.Length == 0)
            displayName = ""
            scriptFlags = OPTION_FLAG_DISABLED
        EndIf
        AddMenuOptionST("SCRIPT_SELECT", "$scs_Short_SCRIPT_SELECT", displayName, scriptFlags)
        AddToggleOptionST("HIDE_INTERFACE", "$scs_Short_HIDE_INTERFACE", HideInterface)
    ElseIf (page == Pages[1])
        AddHeaderOption("$scs_Header_KEY_START_STOP")
        AddEmptyOption()
        AddKeyMapOptionST("KEY_START_STOP", "$scs_Short_KEY_START_STOP", iKeys[START_STOP_SCRIPT], OPTION_FLAG_WITH_UNMAP)
        AddEmptyOption()
        AddKeyMapOptionST("KEY_PREVIOUS", "$scs_Short_KEY_PREVIOUS", iKeys[PREVIOUS_SCRIPT], OPTION_FLAG_WITH_UNMAP)
        AddKeyMapOptionST("KEY_NEXT", "$scs_Short_KEY_NEXT", iKeys[NEXT_SCRIPT], OPTION_FLAG_WITH_UNMAP)
        AddKeyMapOptionST("KEY_RECORDING", "$scs_Short_KEY_RECORDING", iKeys[START_STOP_RECORDING], OPTION_FLAG_WITH_UNMAP)
        AddEmptyOption()
        AddKeyMapOptionST("KEY_RELATIVE_POINT", "$scs_Short_KEY_RELATIVE_POINT", iKeys[RELATIVE_POINT_RECORDING], OPTION_FLAG_WITH_UNMAP)
        AddKeyMapOptionST("KEY_ABSOLUTE_POINT", "$scs_Short_KEY_ABSOLUTE_POINT", iKeys[ABSOLUTE_POINT_RECORDING], OPTION_FLAG_WITH_UNMAP)
        AddTextOptionST("KEYS_CLEAR_ALL", "$scs_Short_KEYS_CLEAR_ALL", "$scs_Value_KEYS_CLEAR_ALL")
    EndIf
EndEvent

Function SelectPreviousScript()
    If (!scriptFiles || scriptFiles.Length == 0)
        Return
    EndIf

    selectedScriptIndex -= 1
    If (selectedScriptIndex < 0)
        selectedScriptIndex = scriptFiles.Length - 1
    EndIf
    SelectedScript = scriptFiles[selectedScriptIndex]
EndFunction

Function SelectNextScript()
    If (!scriptFiles || scriptFiles.Length == 0)
        Return
    EndIf

    selectedScriptIndex += 1
    If (selectedScriptIndex >= scriptFiles.Length)
        selectedScriptIndex = 0
    EndIf
    SelectedScript = scriptFiles[selectedScriptIndex]
EndFunction

State SCRIPT_SELECT
    Event OnMenuOpenST()
        RefreshScriptFiles()
        If (!scriptFiles || scriptFiles.Length == 0)
            Return
        EndIf

        SetMenuDialogOptions(scriptFiles)
        SetMenuDialogStartIndex(selectedScriptIndex)
        SetMenuDialogDefaultIndex(0)
    EndEvent

    Event OnMenuAcceptST(Int index)
        If (!scriptFiles || scriptFiles.Length == 0)
            Return
        EndIf
        If (index < 0 || index >= scriptFiles.Length)
            Return
        EndIf
        selectedScriptIndex = index
        SelectedScript = scriptFiles[index]
        SetMenuOptionValueST(SelectedScript)
    EndEvent

    Event OnHighlightST()
        SetInfoText("$scs_Desc_SCRIPT_SELECT")
    EndEvent
EndState

State HIDE_INTERFACE
    Event OnSelectST()
        HideInterface = !HideInterface
        SetToggleOptionValueST(HideInterface)
    EndEvent

    Event OnDefaultST()
        HideInterface = False
        SetToggleOptionValueST(HideInterface)
    EndEvent

    Event OnHighlightST()
        SetInfoText("$scs_Desc_HIDE_INTERFACE")
    EndEvent
EndState

State KEY_START_STOP
    Event OnKeyMapChangeST(Int keyCode, String conflictControl, String conflictName)
        iKeys[START_STOP_SCRIPT] = keyCode
        SetKeyMapOptionValueST(keyCode)
        NotifyKeyMapChanged()
    EndEvent

    Event OnDefaultST()
        iKeys[START_STOP_SCRIPT] = 0
        SetKeyMapOptionValueST(0)
        NotifyKeyMapChanged()
    EndEvent

    Event OnHighlightST()
        SetInfoText("$scs_Desc_KEY_START_STOP")
    EndEvent
EndState

State KEY_PREVIOUS
    Event OnKeyMapChangeST(Int keyCode, String conflictControl, String conflictName)
        iKeys[PREVIOUS_SCRIPT] = keyCode
        SetKeyMapOptionValueST(keyCode)
        NotifyKeyMapChanged()
    EndEvent

    Event OnDefaultST()
        iKeys[PREVIOUS_SCRIPT] = 0
        SetKeyMapOptionValueST(0)
        NotifyKeyMapChanged()
    EndEvent

    Event OnHighlightST()
        SetInfoText("$scs_Desc_KEY_PREVIOUS")
    EndEvent
EndState

State KEY_NEXT
    Event OnKeyMapChangeST(Int keyCode, String conflictControl, String conflictName)
        iKeys[NEXT_SCRIPT] = keyCode
        SetKeyMapOptionValueST(keyCode)
        NotifyKeyMapChanged()
    EndEvent

    Event OnDefaultST()
        iKeys[NEXT_SCRIPT] = 0
        SetKeyMapOptionValueST(0)
        NotifyKeyMapChanged()
    EndEvent

    Event OnHighlightST()
        SetInfoText("$scs_Desc_KEY_NEXT")
    EndEvent
EndState

State KEY_RECORDING
    Event OnKeyMapChangeST(Int keyCode, String conflictControl, String conflictName)
        iKeys[START_STOP_RECORDING] = keyCode
        SetKeyMapOptionValueST(keyCode)
        NotifyKeyMapChanged()
    EndEvent

    Event OnDefaultST()
        iKeys[START_STOP_RECORDING] = 0
        SetKeyMapOptionValueST(0)
        NotifyKeyMapChanged()
    EndEvent

    Event OnHighlightST()
        SetInfoText("$scs_Desc_KEY_RECORDING")
    EndEvent
EndState

State KEY_RELATIVE_POINT
    Event OnKeyMapChangeST(Int keyCode, String conflictControl, String conflictName)
        iKeys[RELATIVE_POINT_RECORDING] = keyCode
        SetKeyMapOptionValueST(keyCode)
        NotifyKeyMapChanged()
    EndEvent

    Event OnDefaultST()
        iKeys[RELATIVE_POINT_RECORDING] = 0
        SetKeyMapOptionValueST(0)
        NotifyKeyMapChanged()
    EndEvent

    Event OnHighlightST()
        SetInfoText("$scs_Desc_KEY_RELATIVE_POINT")
    EndEvent
EndState

State KEY_ABSOLUTE_POINT
    Event OnKeyMapChangeST(Int keyCode, String conflictControl, String conflictName)
        iKeys[ABSOLUTE_POINT_RECORDING] = keyCode
        SetKeyMapOptionValueST(keyCode)
        NotifyKeyMapChanged()
    EndEvent

    Event OnDefaultST()
        iKeys[ABSOLUTE_POINT_RECORDING] = 0
        SetKeyMapOptionValueST(0)
        NotifyKeyMapChanged()
    EndEvent

    Event OnHighlightST()
        SetInfoText("$scs_Desc_KEY_ABSOLUTE_POINT")
    EndEvent
EndState

State KEYS_CLEAR_ALL
    Event OnSelectST()
        ClearAllKeyBindings()
        SetTextOptionValueST("$scs_Value_KEYS_CLEAR_ALL")
        ForcePageReset()
    EndEvent

    Event OnHighlightST()
        SetInfoText("$scs_Desc_KEYS_CLEAR_ALL")
    EndEvent
EndState
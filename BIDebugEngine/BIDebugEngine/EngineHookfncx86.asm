.386
option casemap :none

_TEXT    SEGMENT


	;mangled functions
	EXTERN ?_scriptEntered@EngineHook@@QAEXI@Z:				PROC;	EngineHook::_scriptEntered
	EXTERN ?_scriptInstruction@EngineHook@@QAEXIIII@Z:		PROC;	EngineHook::_scriptInstruction
	EXTERN ?_scriptLeft@EngineHook@@QAEXI@Z:				PROC;	EngineHook::_scriptLeft
	EXTERN ?_scriptLoaded@EngineHook@@QAEXI@Z:				PROC;	EngineHook::_scriptLoaded
	EXTERN ?_scriptTerminated@EngineHook@@QAEXI@Z:			PROC;	EngineHook::_scriptTerminated
	EXTERN ?_world_OnMissionEventStart@EngineHook@@QAEXI@Z: PROC;	EngineHook::_world_OnMissionEventStart
	EXTERN ?_world_OnMissionEventEnd@EngineHook@@QAEXXZ:	PROC;	EngineHook::_world_OnMissionEventEnd
	EXTERN ?_worldSimulate@EngineHook@@QAEXXZ:				PROC;	EngineHook::_worldSimulate

	;hool Enable fields
	EXTERN _hookEnabled_Instruction:						dword
	EXTERN _hookEnabled_Simulate:							dword

	;JmpBacks

	EXTERN _instructionBreakpointJmpBack:					PROC
	EXTERN _scriptVMSimulateStartJmpBack:					PROC
	EXTERN _worldSimulateJmpBack:							PROC
	EXTERN _worldMissionEventStartJmpBack:					PROC
	EXTERN _worldMissionEventEndJmpBack:					PROC
	EXTERN _scriptVMConstructorJmpBack:						PROC

	;misc
	EXTERN _GlobalEngineHook:								dword
	EXTERN _scriptVM:										dword
	EXTERN _currentScriptVM:								dword

	;##########
	PUBLIC _instructionBreakpoint
	_instructionBreakpoint PROC

		;mov instructionBP_gameState, ebp;
		;mov instructionBP_VMContext, edi;
		;mov instructionBP_Instruction, ebx;
		;push    eax;											don't need to keep because get's overwritten by fixup
		push    ecx;
		mov     ecx, _hookEnabled_Instruction;					Skip if hook is disabled
		test    ecx, ecx;
		jz      _return;
		mov     eax, [esp + 14Ch]; instructionBP_IDebugScript
		push    eax; instructionBP_IDebugScript
		push    ebp; instructionBP_gameState
		push    edi; instructionBP_VMContext
		push    ebx; instructionBP_Instruction
		mov     ecx, offset _GlobalEngineHook;
		call    ?_scriptInstruction@EngineHook@@QAEXIIII@Z;		EngineHook::_scriptInstruction
	_return:
		pop     ecx;
		;pop     eax;
		mov     eax, [ebx + 14h];								fixup
		lea     edx, [ebx + 14h];
		jmp _instructionBreakpointJmpBack;

	_instructionBreakpoint ENDP

	;##########
	PUBLIC _scriptVMConstructor
	_scriptVMConstructor PROC

		push edi;												scriptVM Pointer
        mov ecx, offset _GlobalEngineHook;
        call ?_scriptLoaded@EngineHook@@QAEXI@Z;				EngineHook::_scriptLoaded;
        ;_return:
        push    1;												Fixup
        lea eax, [edi + 298h];
        jmp _scriptVMConstructorJmpBack;

	_scriptVMConstructor ENDP



	IFDEF  passSimulateScriptVMPtr
		.ERR <"hookEnabled_Simulate may kill engine if it's disabled after simulateStart and before simulateEnd">
	ENDIF

	;##########
	PUBLIC _scriptVMSimulateStart
	_scriptVMSimulateStart PROC

		push    eax;
        push    ecx;
		
	IFNDEF passSimulateScriptVMPtr
		mov eax, offset _currentScriptVM;
        mov [eax], ecx;								use this in case of scriptVM ptr not being easilly accessible in SimEnd
	ENDIF
       
        mov     eax, _hookEnabled_Simulate;						Skip if hook is disabled
        test    eax, eax;
        jz      _return;

        push    ecx;											_scriptEntered arg
        mov     ecx, offset _GlobalEngineHook;
        call    ?_scriptEntered@EngineHook@@QAEXI@Z;												EngineHook::_scriptEntered;
    _return:
        pop     ecx;
        pop     eax;
        sub     esp, 34h;										Fixup
        push    edi;
        mov     edi, ecx;
	IFDEF passSimulateScriptVMPtr
        cmp     byte ptr[edi + 2A0h], 0;						if !Loaded we exit right away and never hit scriptVMSimulateEnd
        jz		_skipVMPush;
        push	edi;											scriptVM to receive again in scriptVMSimulateEnd
    _skipVMPush:
	ENDIF
        jmp		_scriptVMSimulateStartJmpBack;
	_scriptVMSimulateStart ENDP


	;##########
	PUBLIC _scriptVMSimulateEnd
	_scriptVMSimulateEnd PROC

        push    eax;
        push    ecx;
        push    edx;

        mov     ecx, _hookEnabled_Simulate;						Skip if hook is disabled
        test    ecx, ecx;
        jz      _return;

        ;prepare arguments for func call
	IFDEF passSimulateScriptVMPtr
        mov     edi, [esp + Ch + 4h/*I added push edx*/];		Retrieve out pushed scriptVM ptr
	ELSE
        mov     edi, _currentScriptVM;							use this in case of scriptVM ptr not being easilly accessible 
	ENDIF
        push    edi;											scriptVM
        mov     ecx, offset _GlobalEngineHook;
        test    al, al;											al == done
        jz      short _notDone;									script is not Done  
        call	?_scriptTerminated@EngineHook@@QAEXI@Z;			EngineHook::_scriptTerminated;	script is Done
        jmp     short _return;
    _notDone:
        call    ?_scriptLeft@EngineHook@@QAEXI@Z;				EngineHook::_scriptLeft;
    _return:
        pop     edx;
        pop     ecx;											These are probably not needed. But I can't guarantee that the compiler didn't expect these to stay unchanged
        pop     eax;
	IFDEF passSimulateScriptVMPtr
        pop     edi;											Remove our pushed scriptVM ptr
	ENDIF
        pop     ebp;											Fixup
        pop     edi;
        add     esp, 34h;
        retn    8;

	_scriptVMSimulateEnd ENDP

	;##########
	PUBLIC _worldSimulate
	_worldSimulate PROC

        push	ecx;
        push	eax;
        mov     ecx, offset _GlobalEngineHook;
        call    ?_worldSimulate@EngineHook@@QAEXXZ;												EngineHook::_worldSimulate;
        pop     eax;											Don't know if eax will be modified but it's likely
        pop     ecx;
        sub     esp, 3D8h;										fixup
        jmp _worldSimulateJmpBack;

	_worldSimulate ENDP


	;##########
	PUBLIC _worldMissionEventStart
	_worldMissionEventStart PROC

        push ecx;
        push eax;

        push	eax;												_world_OnMissionEventStart argument
        mov     ecx, offset _GlobalEngineHook;
        call    ?_world_OnMissionEventStart@EngineHook@@QAEXI@Z;	EngineHook::_world_OnMissionEventStart;
        pop     eax;												Don't know if eax will be modified but it's likely
        pop     ecx;

        push    ebx;												fixup
        mov     ebx, ecx;
        push    esi;
        lea     esi, [eax + eax * 4];
        jmp _worldMissionEventStartJmpBack;

	_worldMissionEventStart ENDP

		;##########
	PUBLIC _worldMissionEventEnd
	_worldMissionEventEnd PROC

        push ecx;
        push eax;
        mov     ecx, offset _GlobalEngineHook;
        call    ?_world_OnMissionEventEnd@EngineHook@@QAEXXZ;	EngineHook::_world_OnMissionEventEnd;
        pop     eax;											Don't know if eax will be modified but it's likely
        pop     ecx;

        pop     edi;											fixup
        pop     esi;
        pop     ebx;
        mov     esp, ebp;
        pop     ebp;
        jmp _worldMissionEventEndJmpBack;

	_worldMissionEventEnd ENDP






_TEXT    ENDS
END

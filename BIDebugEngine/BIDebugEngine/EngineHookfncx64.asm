option casemap :none

_TEXT    SEGMENT
    ;https://msdn.microsoft.com/en-us/library/windows/hardware/ff561499(v=vs.85).aspx

    ;mangled functions
    EXTERN ?_scriptEntered@EngineHook@@QEAAX_K@Z:               PROC;    EngineHook::_scriptEntered
    EXTERN ?_scriptInstruction@EngineHook@@QEAAX_K000@Z:        PROC;    EngineHook::_scriptInstruction
    EXTERN ?_scriptLeft@EngineHook@@QEAAX_K@Z:                  PROC;    EngineHook::_scriptLeft
    EXTERN ?_scriptLoaded@EngineHook@@QEAAX_K@Z:                PROC;    EngineHook::_scriptLoaded
    EXTERN ?_scriptTerminated@EngineHook@@QEAAX_K@Z:            PROC;    EngineHook::_scriptTerminated
    EXTERN ?_world_OnMissionEventStart@EngineHook@@QEAAX_K@Z:   PROC;    EngineHook::_world_OnMissionEventStart
    EXTERN ?_world_OnMissionEventEnd@EngineHook@@QEAAXXZ:       PROC;    EngineHook::_world_OnMissionEventEnd
    EXTERN ?_worldSimulate@EngineHook@@QEAAXXZ:                 PROC;    EngineHook::_worldSimulate
    EXTERN ?_onScriptError@EngineHook@@QEAAX_K@Z:               PROC;    EngineHook::_onScriptError

    ;hool Enable fields    
    EXTERN hookEnabled_Instruction:                             qword
    EXTERN hookEnabled_Simulate:                                qword

    ;JmpBacks

    EXTERN instructionBreakpointJmpBack:                        qword
    EXTERN scriptVMSimulateStartJmpBack:                        qword
    EXTERN worldSimulateJmpBack:                                qword
    EXTERN worldMissionEventStartJmpBack:                       qword
    EXTERN worldMissionEventEndJmpBack:                         qword
    EXTERN scriptVMConstructorJmpBack:                          qword
    EXTERN onScriptErrorJmpBack:                                qword
    EXTERN scriptPreprocessorConstructorJmpBack:               qword
    EXTERN scriptAssertJmpBack:                                qword
    EXTERN scriptHaltJmpBack:                                  qword

    ;misc
    EXTERN GlobalEngineHook:                                    qword
    EXTERN scriptVM:                                            qword
    EXTERN currentScriptVM:                                     qword
    EXTERN scriptPreprocessorDefineDefine:                      qword
    EXTERN preprocMacroName:                                    qword
    EXTERN preprocMacroValue:                                   qword

    ;##########
    ;1.69.140.875 000000000133F0A0
    PUBLIC instructionBreakpoint
    instructionBreakpoint PROC

        push    rcx;
        push    rdx;
        push    r8;
        push    r9;

        mov     rcx, hookEnabled_Instruction;                    Skip if hook is disabled
        test    rcx, rcx;
        jz      _return;
        mov     r9, rbx; instructionBP_gameState
        mov     r8, [rbp+0A0h]; instructionBP_VMContext
        mov     rdx, r12; instructionBP_Instruction

        mov     rcx, offset GlobalEngineHook;
        ; _scriptInstruction( rcx (this), rdx(instructionBP_Instruction), r8(instructionBP_VMContext), r9(instructionBP_gameState))  ;,instructionBP_IDebugScript was optimized out

        call    ?_scriptInstruction@EngineHook@@QEAAX_K000@Z;        EngineHook::_scriptInstruction
    _return:



        pop     r9;
        pop     r8;
        pop     rdx;
        pop     rcx;

        mov     rax, [r13+8]; fixup
        mov     rdx, [rbp+0A0h]
        mov     rcx, r12
        ;mov     rdi, [rdi+rax]
        ;mov     rax, [r12]
        ;mov     esi, [rdx+288h]

        jmp instructionBreakpointJmpBack;

    instructionBreakpoint ENDP
    
    
    ;##########
    PUBLIC scriptVMConstructor
    scriptVMConstructor PROC

        push    rcx;
        push    rdx;
        mov     rdx, rdi;                                        scriptVM Pointer
        mov     rcx, offset GlobalEngineHook;
        call    ?_scriptLoaded@EngineHook@@QEAAX_K@Z;            EngineHook::_scriptLoaded;
        ;_return:

        pop     rdx;
        pop     rcx;
        mov     rax, rdi;                                        Fixup
        add     rsp, 60h
        pop     r15
        pop     r14
        pop     r13
        pop     rdi
        pop     rsi
        pop     rbp
        pop     rbx
        jmp scriptVMConstructorJmpBack;

    scriptVMConstructor ENDP



    IFDEF  passSimulateScriptVMPtr
        .ERR <"hookEnabled_Simulate may kill engine if it's disabled after simulateStart and before simulateEnd">
    ENDIF

    ;##########
    PUBLIC scriptVMSimulateStart
    scriptVMSimulateStart PROC

        push    rax;
        

        mov     rax, hookEnabled_Simulate;                        Skip if hook is disabled
        test    rax, rax;
        jz      _return;
    
        push    rcx;
        push    rdx;

        mov     rdx, rcx;                                        _scriptEntered arg
        mov     rcx, offset GlobalEngineHook;
        call    ?_scriptEntered@EngineHook@@QEAAX_K@Z;            EngineHook::_scriptEntered;
    
    
        pop     rdx;
        pop     rcx;

    _return:

        
        pop     rax;


        push    rbp;                                            Fixup
        push    r14
        lea     rbp, [rsp-4Fh]
        sub     rsp, 98h
        jmp     scriptVMSimulateStartJmpBack;
    scriptVMSimulateStart ENDP

    
    ;##########
    PUBLIC scriptVMSimulateEnd
    scriptVMSimulateEnd PROC

        push rcx;                                                scriptVM is at [rsp+10h]
        

        mov     rcx, hookEnabled_Simulate;                        Skip if hook is disabled
        test    rcx, rcx;
        jz      _return;

        push    rdx;
        mov     rdx, [rsp + 10h + 10h];                            scriptVM
        mov     rcx, offset GlobalEngineHook;

        test    rdi, rdi;                                        rdi == done
        jz      _notDone;                                        script is not Done  
        call    ?_scriptTerminated@EngineHook@@QEAAX_K@Z;        EngineHook::_scriptTerminated;    script is Done
        jmp     _returnPostCall;
    _notDone:
        call    ?_scriptLeft@EngineHook@@QEAAX_K@Z;                EngineHook::_scriptLeft;

    _returnPostCall:
        pop     rdx;

    _return:
        pop     rcx;

        movaps  xmm6, [rsp+98h+28h]
        movzx   eax, r14b
        add     rsp, 98h
        pop     r14
        pop     rbp
        ret
    scriptVMSimulateEnd ENDP
    
    ;##########
    PUBLIC worldSimulate
    worldSimulate PROC
        ;rcx is worldPtr!
        push    rcx;

        mov     rcx, offset GlobalEngineHook;
        call    ?_worldSimulate@EngineHook@@QEAAXXZ;                EngineHook::_worldSimulate;

        pop     rcx;
        mov     rax, rsp;                                            Fixup
        mov     [rax+20h], r9
        mov     [rax+18h], r8
        mov     [rax+8h], rcx; //This is worldPtr!                                    
        jmp     worldSimulateJmpBack;

    worldSimulate ENDP

    COMMENT ü
    ;##########
    PUBLIC _worldMissionEventStart
    _worldMissionEventStart PROC

        push    ecx;
        push    eax;

        push    eax;                                                _world_OnMissionEventStart argument
        mov     ecx, offset _GlobalEngineHook;
        call    ?_world_OnMissionEventStart@EngineHook@@QAEXI@Z;    EngineHook::_world_OnMissionEventStart;
        pop     eax;                                                Don't know if eax will be modified but it's likely
        pop     ecx;

        push    ebx;                                                fixup
        mov     ebx, ecx;
        push    esi;
        lea     esi, [eax + eax * 4];
        jmp _worldMissionEventStartJmpBack;

    _worldMissionEventStart ENDP

    ;##########
    PUBLIC _worldMissionEventEnd
    _worldMissionEventEnd PROC

        push    ecx;
        push    eax;
        mov     ecx, offset _GlobalEngineHook;
        call    ?_world_OnMissionEventEnd@EngineHook@@QAEXXZ;    EngineHook::_world_OnMissionEventEnd;
        pop     eax;                                            Don't know if eax will be modified but it's likely
        pop     ecx;

        pop     edi;                                            fixup
        pop     esi;
        pop     ebx;
        mov     esp, ebp;
        pop     ebp;
        jmp _worldMissionEventEndJmpBack;

    _worldMissionEventEnd ENDP

    ü;

    ;##########
    PUBLIC onScriptError
    onScriptError PROC

        push    rcx;
        push    rdx;

        mov     rdx, rcx;                                           gameState Ptr
        mov     rcx, offset GlobalEngineHook;
        call    ?_onScriptError@EngineHook@@QEAAX_K@Z;              EngineHook::_onScriptError;
        
        pop     rdx;
        pop     rcx;                                                

        mov     rax, rsp;                                           Fixup
        mov     [rax+8], rcx
        push    rbp
        lea     rbp, [rax-5Fh]
        sub     rsp, 90h
        jmp     onScriptErrorJmpBack;
    onScriptError ENDP

     ;##########
    PUBLIC scriptPreprocessorConstructor
    scriptPreprocessorConstructor PROC



        mov     [rbx+8], rdi
        mov     [rbx+28h], rdi
        mov     [rbx+10h], edi
        mov     dword ptr [rbx+0C4h], 14h
        mov     rax, rbx

        ;push rax;
        push rdx; 

        mov     r8, preprocMacroValue;
        mov     rdx, preprocMacroName;
        mov     rcx, rax;                                           this*
        mov     rbx, scriptPreprocessorDefineDefine;
        call    rbx;
        
        pop rdx;
        ;pop rax;

        jmp     scriptPreprocessorConstructorJmpBack;

    scriptPreprocessorConstructor ENDP





_TEXT    ENDS
END

#include "SQF-Assembly-Iface.h"
#include <intercept.hpp>



static struct {
	void* vt_GameInstructionNewExpression;
	void* vt_GameInstructionConst;
	void* vt_GameInstructionFunction;
	void* vt_GameInstructionOperator;
	void* vt_GameInstructionAssignment;
	void* vt_GameInstructionVariable;
	void* vt_GameInstructionArray;
} oldFunc;



static struct vtables {
	void** vt_GameInstructionNewExpression;
	void** vt_GameInstructionConst;
	void** vt_GameInstructionFunction;
	void** vt_GameInstructionOperator;
	void** vt_GameInstructionAssignment;
	void** vt_GameInstructionVariable;
	void** vt_GameInstructionArray;
} GVt;


SQF_Assembly_Iface::instructionExecFnc h_GameInstructionNewExpression{ nullptr };
SQF_Assembly_Iface::instructionExecFnc h_GameInstructionConst{ nullptr };
SQF_Assembly_Iface::instructionExecFnc h_GameInstructionFunction{ nullptr };
SQF_Assembly_Iface::instructionExecFnc h_GameInstructionOperator{ nullptr };
SQF_Assembly_Iface::instructionExecFnc h_GameInstructionAssignment{ nullptr };
SQF_Assembly_Iface::instructionExecFnc h_GameInstructionVariable{ nullptr };
SQF_Assembly_Iface::instructionExecFnc h_GameInstructionArray{ nullptr };

#define CALL_HOOK(x) if (h_##x) h_##x(this,state,t)

class GameInstructionConst : public game_instruction {
public:
	game_value value;
	virtual bool exec(game_state& state, vm_context& t) {
		CALL_HOOK(GameInstructionConst);

		typedef bool(__thiscall *OrigEx)(game_instruction*, game_state&, vm_context&);
		return reinterpret_cast<OrigEx>(oldFunc.vt_GameInstructionConst)(this, state, t);
	}
	virtual int stack_size(void* t) const { return 0; }
	virtual r_string get_name() const { return ""sv; }
};

class GameInstructionVariable : public game_instruction {
public:
	r_string name;
	virtual bool exec(game_state& state, vm_context& t) {
		CALL_HOOK(GameInstructionVariable);

		typedef bool(__thiscall *OrigEx)(game_instruction*, game_state&, vm_context&);
		return reinterpret_cast<OrigEx>(oldFunc.vt_GameInstructionVariable)(this, state, t);
	}
	virtual int stack_size(void* t) const { return 0; }
	virtual r_string get_name() const { return ""sv; }
};

class GameInstructionOperator : public game_instruction {
public:
	const intercept::__internal::game_operators *_operators;
	virtual bool exec(game_state& state, vm_context& t) {
		CALL_HOOK(GameInstructionOperator);

		typedef bool(__thiscall *OrigEx)(game_instruction*, game_state&, vm_context&);
		return reinterpret_cast<OrigEx>(oldFunc.vt_GameInstructionOperator)(this, state, t);
	}
	virtual int stack_size(void* t) const { return 0; }
	virtual r_string get_name() const { return ""sv; }
};

class GameInstructionFunction : public game_instruction {
public:
	const intercept::__internal::game_functions *_functions;
	virtual bool exec(game_state& state, vm_context& t) {
		CALL_HOOK(GameInstructionFunction);

		typedef bool(__thiscall *OrigEx)(game_instruction*, game_state&, vm_context&);
		return reinterpret_cast<OrigEx>(oldFunc.vt_GameInstructionFunction)(this, state, t);
	}
	virtual int stack_size(void* t) const { return 0; }
	virtual r_string get_name() const { return ""sv; }
};

class GameInstructionArray : public game_instruction {
public:
	int size;
	virtual bool exec(game_state& state, vm_context& t) {
		CALL_HOOK(GameInstructionArray);

		typedef bool(__thiscall *OrigEx)(game_instruction*, game_state&, vm_context&);
		return reinterpret_cast<OrigEx>(oldFunc.vt_GameInstructionArray)(this, state, t);
	}
	virtual int stack_size(void* t) const { return 0; }
	virtual r_string get_name() const { return ""sv; }
};

class GameInstructionAssignment : public game_instruction {
public:
	r_string name;
	bool forceLocal;
	virtual bool exec(game_state& state, vm_context& t) {
		CALL_HOOK(GameInstructionAssignment);

		typedef bool(__thiscall *OrigEx)(game_instruction*, game_state&, vm_context&);
		return reinterpret_cast<OrigEx>(oldFunc.vt_GameInstructionAssignment)(this, state, t);
	}
	virtual int stack_size(void* t) const { return 0; }
	virtual r_string get_name() const { return ""sv; }
};

class GameInstructionNewExpression : public game_instruction {
public:
	int beg{ 0 };
	int end{ 0 };
	virtual bool exec(game_state& state, vm_context& t) {
		CALL_HOOK(GameInstructionNewExpression);

		typedef bool(__thiscall *OrigEx)(game_instruction*, game_state&, vm_context&);
		return reinterpret_cast<OrigEx>(oldFunc.vt_GameInstructionNewExpression)(this, state, t);
	}
	virtual int stack_size(void* t) const { return 0; }
	virtual r_string get_name() const { return ""sv; }
};



void SQF_Assembly_Iface::init() {
	auto iface = intercept::client::host::request_plugin_interface("sqf_asm_devIf", 1);
	if (!iface) {
		ready = false;
		return;
	}
	GVt = *static_cast<vtables*>(*iface);



	DWORD dwVirtualProtectBackup;
	//From ArmaScriptProfiler - Written by me

	game_instruction* ins = new GameInstructionConst();
	VirtualProtect(reinterpret_cast<LPVOID>(GVt.vt_GameInstructionConst), 14u, 0x40u, &dwVirtualProtectBackup);
	oldFunc.vt_GameInstructionConst = GVt.vt_GameInstructionConst[3];
	GVt.vt_GameInstructionConst[3] = (*reinterpret_cast<void***>(ins))[3];
	VirtualProtect(reinterpret_cast<LPVOID>(GVt.vt_GameInstructionConst), 14u, dwVirtualProtectBackup, &dwVirtualProtectBackup);
	delete ins;

	ins = new GameInstructionVariable();
	VirtualProtect(reinterpret_cast<LPVOID>(GVt.vt_GameInstructionVariable), 14u, 0x40u, &dwVirtualProtectBackup);
	oldFunc.vt_GameInstructionVariable = GVt.vt_GameInstructionVariable[3];
	GVt.vt_GameInstructionVariable[3] = (*reinterpret_cast<void***>(ins))[3];
	VirtualProtect(reinterpret_cast<LPVOID>(GVt.vt_GameInstructionVariable), 14u, dwVirtualProtectBackup, &dwVirtualProtectBackup);
	delete ins;

	ins = new GameInstructionOperator();
	VirtualProtect(reinterpret_cast<LPVOID>(GVt.vt_GameInstructionOperator), 14u, 0x40u, &dwVirtualProtectBackup);
	oldFunc.vt_GameInstructionOperator = GVt.vt_GameInstructionOperator[3];
	GVt.vt_GameInstructionOperator[3] = (*reinterpret_cast<void***>(ins))[3];
	VirtualProtect(reinterpret_cast<LPVOID>(GVt.vt_GameInstructionOperator), 14u, dwVirtualProtectBackup, &dwVirtualProtectBackup);
	delete ins;

	ins = new GameInstructionFunction();
	VirtualProtect(reinterpret_cast<LPVOID>(GVt.vt_GameInstructionFunction), 14u, 0x40u, &dwVirtualProtectBackup);
	oldFunc.vt_GameInstructionFunction = GVt.vt_GameInstructionFunction[3];
	GVt.vt_GameInstructionFunction[3] = (*reinterpret_cast<void***>(ins))[3];
	VirtualProtect(reinterpret_cast<LPVOID>(GVt.vt_GameInstructionFunction), 14u, dwVirtualProtectBackup, &dwVirtualProtectBackup);
	delete ins;

	ins = new GameInstructionArray();
	VirtualProtect(reinterpret_cast<LPVOID>(GVt.vt_GameInstructionArray), 14u, 0x40u, &dwVirtualProtectBackup);
	oldFunc.vt_GameInstructionArray = GVt.vt_GameInstructionArray[3];
	GVt.vt_GameInstructionArray[3] = (*reinterpret_cast<void***>(ins))[3];
	VirtualProtect(reinterpret_cast<LPVOID>(GVt.vt_GameInstructionArray), 14u, dwVirtualProtectBackup, &dwVirtualProtectBackup);
	delete ins;

	ins = new GameInstructionAssignment();
	VirtualProtect(reinterpret_cast<LPVOID>(GVt.vt_GameInstructionAssignment), 14u, 0x40u, &dwVirtualProtectBackup);
	oldFunc.vt_GameInstructionAssignment = GVt.vt_GameInstructionAssignment[3];
	GVt.vt_GameInstructionAssignment[3] = (*reinterpret_cast<void***>(ins))[3];
	VirtualProtect(reinterpret_cast<LPVOID>(GVt.vt_GameInstructionAssignment), 14u, dwVirtualProtectBackup, &dwVirtualProtectBackup);
	delete ins;

	ready = true;
}

void SQF_Assembly_Iface::setHook(InstructionType type, instructionExecFnc func) {
	switch (type) {
		case InstructionType::GameInstructionNewExpression: h_GameInstructionNewExpression = func; break;
		case InstructionType::GameInstructionConst: h_GameInstructionConst = func; break;
		case InstructionType::GameInstructionFunction: h_GameInstructionFunction = func; break;
		case InstructionType::GameInstructionOperator: h_GameInstructionOperator = func; break;
		case InstructionType::GameInstructionAssignment: h_GameInstructionAssignment = func; break;
		case InstructionType::GameInstructionVariable: h_GameInstructionVariable = func; break;
		case InstructionType::GameInstructionArray: h_GameInstructionArray = func; break;
		default: __debugbreak();
	}
}

#include "Debugger.h"
#include "VMContext.h"
#include <fstream>

Debugger::Debugger() {}


Debugger::~Debugger() {}

void Debugger::clear() {
    if (VMPtrToScript.empty()) return;
    std::vector<uintptr_t> to_delete;
    to_delete.reserve(VMPtrToScript.size());
    for (auto&it : VMPtrToScript) {
        if (it.second->canBeDeleted)
            to_delete.push_back(it.first);
    }
    if (to_delete.size() == VMPtrToScript.size()) {
        VMPtrToScript.clear();
        return;
    }
    for (auto it : to_delete)
        VMPtrToScript.erase(it);


}

std::shared_ptr<VMContext> Debugger::getVMContext(RV_VMContext* vm) {
    uintptr_t contentPtr = reinterpret_cast<uintptr_t>(vm);
    auto iter(VMPtrToScript.lower_bound(contentPtr));
    //Thank you SO! 
    if (iter == VMPtrToScript.end() || contentPtr < iter->first) {    // not found
        auto newEntry = std::make_shared<VMContext>();
        VMPtrToScript.insert(iter, { contentPtr, newEntry });// hinted insertion
        return newEntry;
    }
    return iter->second;
}

void Debugger::writeFrameToFile(uint32_t frameCounter) {

    std::ofstream file("P:\\" + std::to_string(frameCounter) + ".txt");

    if (file.is_open()) {

        for (auto& it : VMPtrToScript) {
            file << "VM " << it.second->totalRuntime.count() / 1000 << "us isVM:" << it.second->isScriptVM << "\n";

            for (auto& instruction : it.second->instructions) {
                uint16_t tabs = 1;
                instruction.writeToFile(file, tabs);
            }
        }
    }
    file.flush();
}

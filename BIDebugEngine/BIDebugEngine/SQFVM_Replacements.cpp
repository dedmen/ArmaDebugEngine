#include <type.h>
#include <virtualmachine.h>


#include "filesystem.h"

std::string sqf::type_str(type) {return "";}


std::optional<std::string> sqf::filesystem::try_get_physical_path(std::string_view virt, std::string_view current) {
    if (virt.empty()) {
        return {};
    }

	std::string virtMapping;
    if (virt.front() != '\\') { //It's a local path
        auto parentDirectory = std::filesystem::path(current).parent_path(); //Get parent of current file
        auto wantedFile = (parentDirectory / virt).lexically_normal();

        auto absolute = std::filesystem::absolute(wantedFile);
        return absolute.string();
    } else { //global path
        return std::string(virt);
    }
}

sqf::virtualmachine::virtualmachine(Logger& logger, unsigned long long maxinst) : CanLog(logger) {}
sqf::virtualmachine::~virtualmachine() {}

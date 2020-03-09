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

        return wantedFile.string();
    } else { //global path
        return std::string(virt);
    }
}

sqf::virtualmachine::virtualmachine(Logger& logger, unsigned long long maxinst) : CanLog(logger) {}
sqf::virtualmachine::~virtualmachine() {}

#include "fileio.h"
#include <intercept.hpp>

std::string load_file(const std::string& filename)
{
    return std::string(intercept::sqf::load_file(filename));
}

std::vector<char> readFile(const std::string& filename)
{
    std::vector<char> data;
    auto res = intercept::sqf::load_file(filename);
    data.reserve(res.length());
    std::copy(res.begin(), res.end(), std::back_inserter(data));
    return data;
}

int get_bom_skip(const std::vector<char>& buff)
{
    if (buff.empty())
        return 0;
    // We are comparing against unsigned
    auto ubuff = reinterpret_cast<const unsigned char*>(buff.data());
    if (ubuff[0] == 0xEF && ubuff[1] == 0xBB && ubuff[2] == 0xBF)
    {
        //UTF-8
        return 3;
    }
    else if (ubuff[0] == 0xFE && ubuff[1] == 0xFF)
    {
        //UTF-16 (BE)
        return 2;
    }
    else if (ubuff[0] == 0xFE && ubuff[1] == 0xFE)
    {
        //UTF-16 (LE)
        return 2;
    }
    else if (ubuff[0] == 0x00 && ubuff[1] == 0x00 && ubuff[2] == 0xFF && ubuff[3] == 0xFF)
    {
        //UTF-32 (BE)
        return 2;
    }
    else if (ubuff[0] == 0xFF && ubuff[1] == 0xFF && ubuff[2] == 0x00 && ubuff[3] == 0x00)
    {
        //UTF-32 (LE)
        return 2;
    }
    else if (ubuff[0] == 0x2B && ubuff[1] == 0x2F && ubuff[2] == 0x76 &&
        (ubuff[3] == 0x38 || ubuff[3] == 0x39 || ubuff[3] == 0x2B || ubuff[3] == 0x2F))
    {
        //UTF-7
        return 4;
    }
    else if (ubuff[0] == 0xF7 && ubuff[1] == 0x64 && ubuff[2] == 0x4C)
    {
        //UTF-1
        return 3;
    }
    else if (ubuff[0] == 0xDD && ubuff[1] == 0x73 && ubuff[2] == 0x66 && ubuff[3] == 0x73)
    {
        //UTF-EBCDIC
        return 3;
    }
    else if (ubuff[0] == 0x0E && ubuff[1] == 0xFE && ubuff[2] == 0xFF)
    {
        //SCSU
        return 3;
    }
    else if (ubuff[0] == 0xFB && ubuff[1] == 0xEE && ubuff[2] == 0x28)
    {
        //BOCU-1
        if (ubuff[3] == 0xFF)
            return 4;
        return 3;
    }
    else if (ubuff[0] == 0x84 && ubuff[1] == 0x31 && ubuff[2] == 0x95 && ubuff[3] == 0x33)
    {
        //GB 18030
        return 3;
    }
    return 0;
}

extern bool (*fileExists)(r_string);
bool file_exists(const std::string& filename)
{
    if (fileExists) return fileExists(r_string(filename));
    auto res = intercept::sqf::load_file(filename);
    return !res.empty();
}

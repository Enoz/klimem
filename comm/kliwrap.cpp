#include "./comm.c"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <unistd.h>
#include <vector>

namespace KLiMem {

struct ModuleInfo {
    void *base;
    size_t size;
};

std::vector<std::string> split(std::string s, std::string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(s.substr(pos_start));
    return res;
}

class Memory {
    int fd;

  public:
    Memory() {
        fd = open(DEVICE_NAME, O_RDWR);
        if (fd < 0) {
            throw "Unable to find KLiMem character device";
        }
    }
    ~Memory() { close(fd); }

    void KReadProcessMemory(pid_t pid, void *address, size_t read_size,
                            void *buffer) {

        struct T_RPM rpm_request;
        rpm_request.target_pid = pid;
        rpm_request.target_address = (unsigned long)address;
        rpm_request.read_size = read_size;
        rpm_request.buffer_address = (unsigned long)buffer;

        if (ioctl(fd, IOCTL_RPM, &rpm_request) < 0) {
            throw "KLiMem failed to read memory";
        }
    }

    std::unique_ptr<T_PROCESSES> GetProcesses() {
        std::unique_ptr<T_PROCESSES> procs = std::make_unique<T_PROCESSES>();
        T_PROCESSES *ptr = procs.get();
        if (ioctl(fd, IOCTL_GET_PROCESSES, (unsigned long)(&ptr)) < 0) {
            throw "KLiMem failed to read processes";
        }
        return procs;
    }

    std::unique_ptr<T_MODULES> GetProcessModules(pid_t pid) {
        std::unique_ptr<T_MODULES> mods = std::make_unique<T_MODULES>();

        struct T_MODULE_REQUEST modReq;
        modReq.pid = pid;
        modReq.buffer_address = (unsigned long)mods.get();

        if (ioctl(fd, IOCTL_GET_MODULES, &modReq) < 0) {
            throw "KLiMem failed to read module maps";
        }
        return mods;
    }

    ModuleInfo GetModuleBase(pid_t pid, std::string moduleName) {
        auto mods = GetProcessModules(pid);
        auto modVec = std::vector<T_MODULE>();
        for (int i = 0; i < mods->numModules; i++) {
            auto str = std::string(mods->modules[i].path);
            if (str.find(moduleName) != std::string::npos) {
                modVec.push_back(mods->modules[i]);
            }
        }

        ModuleInfo mi;

        if (modVec.size() == 0)
            return mi;
        std::sort(modVec.begin(), modVec.end(),
                  [](T_MODULE a, T_MODULE b) { return a.start < b.start; });
        mi.base = (void *)modVec[0].start;
        mi.size = modVec[modVec.size() - 1].end - modVec[0].start;
        return mi;
    }

    std::vector<void *> SigScan(pid_t pid, std::string pattern,
                                std::string module) {
        std::vector<unsigned char> patternBytes{};
        std::vector<std::string> patternSplit = split(pattern, " ");
        std::vector<void *> hits{};

        for (std::string sByte : patternSplit) {
            if (sByte.find("?") != std::string::npos) {
                patternBytes.push_back((unsigned char)0);
            } else {
                patternBytes.push_back(
                    (unsigned char)std::stoul(sByte, nullptr, 16));
            }
        }

        ModuleInfo modInfo = this->GetModuleBase(pid, module);
        if (modInfo.size == 0 || modInfo.base == 0) {
            return hits;
        }

        std::unique_ptr<unsigned char[]> moduleBytes(
            new unsigned char[modInfo.size]);
        this->KReadProcessMemory(pid, modInfo.base, modInfo.size,
                                 moduleBytes.get());

        for (int i = 0; i < modInfo.size - patternSplit.size() - 1; i++) {
            bool match = true;
            for (int j = 0; j < patternSplit.size(); j++) {
                std::string patternString = patternSplit[j];
                if (patternString.find("?") != std::string::npos) {
                    continue;
                }

                unsigned char moduleByte = moduleBytes[i + j];
                unsigned char patternByte = patternBytes[j];

                /*printf("%i == %i\n", moduleByte, patternByte);*/

                if (moduleByte != patternByte) {
                    match = false;
                    break;
                }
            }
            if (match) {
                hits.push_back(
                    (void *)((unsigned long)modInfo.base + (unsigned long)i));
            }
        }

        return hits;
    }
};

}; // namespace KLiMem

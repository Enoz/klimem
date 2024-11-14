#include "./comm.c"
#include <algorithm>
#include <memory>
#include <string>
#include <unistd.h>
#include <vector>

namespace KLiMem {
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
        rpm_request.buffer_address = (unsigned long)address;

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

    void *GetModuleBase(pid_t pid, std::string moduleName) {
        auto mods = GetProcessModules(pid);
        auto modVec = std::vector<T_MODULE>();
        for (int i = 0; i < mods->numModules; i++) {
            auto str = std::string(mods->modules[i].path);
            if (str.find(moduleName) != std::string::npos) {
                modVec.push_back(mods->modules[i]);
            }
        }

        if (modVec.size() == 0)
            return 0;
        std::sort(modVec.begin(), modVec.end(),
                  [](T_MODULE a, T_MODULE b) { return a.start < b.start; });
        return (void *)modVec[0].start;
    }
};

}; // namespace KLiMem

# klimem

**K**ernel **Li**nux **Mem**ory is a POC linux kernel module demonstrating how virtual memory can be arbitrarily manipulated by a user space process.

## Capabilities
- Retrieve process list (including kernel-level processes)
- Retrieve process memory maps
- Arbitrarily read process memory from any system process

## Usage

Install Kernel Module
```bash
cd kernel
./install.sh
```

Consult the usermod POC within the project for how to communicate with the kernel module. Additionally, here is a sample wrapper as an example
```cpp
#include <algorithm>

namespace KLiMem {

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

Memory::Memory() {
    fd = open(DEVICE_NAME, O_RDWR);
    if (fd < 0) {
        throw "Unable to find KLiMem character device";
    }
}
Memory::~Memory() { close(fd); }

void Memory::KReadProcessMemory(pid_t pid, unsigned long address,
                                size_t read_size, void *buffer) {

    struct T_RPM rpm_request;
    rpm_request.target_pid = pid;
    rpm_request.target_address = address;
    rpm_request.read_size = read_size;
    rpm_request.buffer_address = (unsigned long)buffer;

    if (ioctl(fd, IOCTL_RPM, &rpm_request) < 0) {
        throw "KLiMem failed to read memory";
    }
}

std::unique_ptr<T_PROCESSES> Memory::GetProcesses() {
    std::unique_ptr<T_PROCESSES> procs = std::make_unique<T_PROCESSES>();
    T_PROCESSES *ptr = procs.get();
    if (ioctl(fd, IOCTL_GET_PROCESSES, (unsigned long)(&ptr)) < 0) {
        throw "KLiMem failed to read processes";
    }
    return procs;
}

std::unique_ptr<T_MODULES> Memory::GetProcessModules(pid_t pid) {
    std::unique_ptr<T_MODULES> mods = std::make_unique<T_MODULES>();

    struct T_MODULE_REQUEST modReq;
    modReq.pid = pid;
    modReq.buffer_address = (unsigned long)mods.get();

    if (ioctl(fd, IOCTL_GET_MODULES, &modReq) < 0) {
        throw "KLiMem failed to read module maps";
    }
    return mods;
}

ModuleInfo Memory::GetModuleBase(pid_t pid, std::string moduleName) {
    auto mods = GetProcessModules(pid);
    auto modVec = std::vector<T_MODULE>();
    for (unsigned int i = 0; i < mods->numModules; i++) {
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
    mi.base = modVec[0].start;
    mi.size = modVec[modVec.size() - 1].end - modVec[0].start;
    return mi;
}

}; // namespace KLiMem
```


## Dependencies
- `gcc`
- `linux-headers` (or non-arch equivalent)
- `make`
- `bear` (optional for clangd lsp)

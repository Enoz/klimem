// #include "../comm/comm.c"
#include "../comm/kliwrap.cpp"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>

int main() {

    auto kmem = std::make_shared<KLiMem::Memory>();

    auto processes = kmem->GetProcesses();

    pid_t cs2_pid;
    for (int i = 0; i < processes->numProcesses; i++) {
        auto proc = processes->processes[i];

        auto procstr = std::string(proc.name);
        if (procstr == "cs2") {
            cs2_pid = proc.pid;
            break;
        }
    }

    printf("CS2 pid: %i\n", cs2_pid);

    auto modules = kmem->GetProcessModules(cs2_pid);
    printf("Found %i modules\n", modules->numModules);
    for (int i = 0; i < modules->numModules; i++) {
        auto mod = modules->modules[i];
        printf("(%p - %p) %s\n", (void *)mod.start, (void *)mod.end, mod.path);
    }

    return 0;
}

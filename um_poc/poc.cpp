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

    /*auto modules = kmem->GetProcessModules(cs2_pid);*/
    /*printf("Found %i modules\n", modules->numModules);*/
    /*for (int i = 0; i < modules->numModules; i++) {*/
    /*    auto mod = modules->modules[i];*/
    /*    printf("(%p - %p) %s\n", (void *)mod.start, (void *)mod.end, mod.path);*/
    /*}*/

    /*void* libclientbase = kmem->GetModuleBase(cs2_pid, "libclient.so");*/
    /*printf("libclient.so base: %p\n", libclientbase);*/

    auto modInfo = kmem->GetModuleBase(cs2_pid, "libclient.so");
    printf("libclient.so base: %p\n", modInfo.base);

    std::string cl_showents_pattern = "55 48 89 e5 41 57 41 56 ?? ?? ?? ?? ?? ?? ?? 41 55 41 54 45 31 e4 53 48 81 ec ?? ??";
    auto scanRes = kmem->SigScan(cs2_pid, cl_showents_pattern, "libclient.so");

    printf("%zu sigscan results\n", scanRes.size());
    for(auto res : scanRes) {
        printf("\t%p (%p + %p)\n", res, modInfo.base, (void*)((unsigned long)res-(unsigned long)modInfo.base));
    }

    return 0;
}

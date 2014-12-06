/**
 * Author: Avidan Borisov, 2014
 * 
 * A highly effective Linux "Process War" combatant
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csetjmp>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/time.h>
#include <linux/limits.h>

class Config {
public:
    static constexpr const char fakeName[] = "uwotm8";      // The program's fake name after hide()
    static constexpr const char searchPath[] = "/tmp";      // The destroy() search path
    static constexpr const int forkDelayMicrosecs = 100000; // The delay between each fork in microsecs
    static constexpr const int maxProcesses = 20;           // Max number of processes running at all times
    static char newPath[PATH_MAX + 1];
};

constexpr const char Config::fakeName[];
constexpr const char Config::searchPath[];
constexpr const int Config::forkDelayMicrosecs;
constexpr const int Config::maxProcesses;
char Config::newPath[PATH_MAX + 1] = "";

// Hide own data from /proc, specifically:
// * Change the path of the image of the process
// * Change the current working directory of the process
// * Change the "name" of the process - it's argument vector
//
// The last two are done easily by calling chdir() and specifying
// a different argv. The hardest one, which is the first, is done by
// making a hardlink to this program somewhere else, and then executing
// that hardlink, which executes the same program, but now with a different
// image path.
void hide() {
    const char* home = std::getenv("HOME");
    std::snprintf(Config::newPath, sizeof Config::newPath, "%s/%s", home, Config::fakeName);

    char path[PATH_MAX + 1] = "";
    readlink("/proc/self/exe", path, sizeof path); // get own image path
    if (std::strcmp(path, Config::newPath) == 0) // quit if we're in the new process already
        return;

    unlink(Config::newPath); // overwrite the hardlink in case it exists already
    link(path, Config::newPath); // create a hardlink to this program in $HOME
    chdir("/"); // change the current working directory to the root directory
    execl(Config::newPath, Config::fakeName, nullptr); // execute this program through the hardlink
}

// Simple aigaction wrapper cause why not man
class Signal {
public:
    using SimpleHandler = void(int);
    using WithInfoHandler = void(int, siginfo_t*, void*);

    explicit Signal(SimpleHandler* handler) {
        sa.sa_handler = handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
    }

    explicit Signal(WithInfoHandler* handler) {
        sa.sa_sigaction = handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART | SA_SIGINFO;
    }

    void handle(int sig) const {
        sigaction(sig, &sa, nullptr);
    }

private:
    struct sigaction sa;
};

// Handle all signals that can terminate this program, and if possible,
// kill the sender of the signal.
void protect() {
    Signal killer([](int, siginfo_t* info, void*) {
        if (info->si_pid > 0 && info->si_pid != getpid()) // if the sender is valid and isn't us
            kill(info->si_pid, SIGKILL); // kill the sender
    });
    
    // No SIGALRM here, it's used in destroy()
    killer.handle(SIGTERM);
    killer.handle(SIGQUIT);
    killer.handle(SIGINT);
    killer.handle(SIGHUP);
    killer.handle(SIGSEGV);
    killer.handle(SIGILL);
    killer.handle(SIGABRT);
    killer.handle(SIGBUS);
    killer.handle(SIGFPE);
    killer.handle(SIGPIPE);
    killer.handle(SIGTSTP);
    killer.handle(SIGTTIN);
    killer.handle(SIGTTOU);
    killer.handle(SIGUSR1);
    killer.handle(SIGUSR2);
    killer.handle(SIGPOLL);
    killer.handle(SIGPROF);
    killer.handle(SIGSYS);
    killer.handle(SIGTRAP);
    killer.handle(SIGVTALRM);
    killer.handle(SIGXCPU);
    killer.handle(SIGXFSZ);
}

class PeriodicTimer {
public:
    static void start(long microsecs, Signal::SimpleHandler* event) {
        Signal handler(event);
        handler.handle(SIGALRM);

        PeriodicTimer::enable(microsecs);
    }

    static void enable(long microsecs) {
        long secs = microsecs / 1000000;
        microsecs %= 1000000;

        itimerval periodic = { { secs, microsecs }, { secs, microsecs } };
        setitimer(ITIMER_REAL, &periodic, nullptr);
    }
};

class SharedMemory {
public:
    void create(const char* id) {
        key_t key = ftok(id, 'A');
        shmid = shmget(key, 1, 0666 | IPC_CREAT);
        shmat(shmid, nullptr, 0);
    }

    // Return the number of processes using this shared memory.
    int numAttached(void) const {
        shmid_ds info;
        shmctl(shmid, IPC_STAT, &info);
        return info.shm_nattch;
    }

private:
    int shmid;
};

static SharedMemory mem;
static sigjmp_buf start;

// Destroy all process from 'Config::searchPath' concurrently, with 'Config::maxProcesses'
// always running at the same time (even one one is killed, another will be forked automatically)
void destroy() {
    mem.create(Config::newPath); // Init with the unique ID of the path of the program itself!

    // Always fork periodically new processes to assist with the destruction and keep
    // our program alive. This is exponential, so we must not create too much
    // to not fork-bomb the system. We use a delay between forks.
    PeriodicTimer::start(Config::forkDelayMicrosecs, [](int) {
        if (mem.numAttached() < Config::maxProcesses) {
            if (fork() == 0) { // if new child
                setpgid(0, 0); // seperate it from the parent's process group
                PeriodicTimer::enable(Config::forkDelayMicrosecs); // create the timer again for the child
                siglongjmp(start, 1); // jump out of the handler back to the loop start
            }
        }
    });

    sigsetjmp(start, 1); // forked process will return to here, and read /proc all over again
    while (true) {
        DIR* proc = opendir("/proc");
        
        while (dirent* entry = readdir(proc)) {
            int pid = std::atoi(entry->d_name);
            if (pid == 0)
                continue; // not a process
            
            // Check if /proc/[pid]/exe is from 'path'. If so, kill it with fire
            char exeLink[PATH_MAX + 1] = "";
            std::snprintf(exeLink, sizeof exeLink, "/proc/%s/exe", entry->d_name);
            char exe[PATH_MAX + 1] = "";
            if (readlink(exeLink, exe, sizeof exe) == -1)
                continue; // can't read location

            if (std::strncmp(exe, Config::searchPath, sizeof Config::searchPath - 1) == 0) { // if 'exe' starts with 'path'
                pid_t group = getpgid(pid);
                kill(group != -1 ? -group : pid, SIGKILL); // kill it's entire process group if possible (ha ha)
            }
        }
        
        closedir(proc);
    }
}

int main() {
    while (access("/tmp/GO", F_OK)) { }

    hide();
    protect();
    destroy();
}

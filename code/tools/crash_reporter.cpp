// Isolated macOS stack collector. No game state or log mutex is shared.
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char** environ;
int main(int argc, char** argv) {
    if (argc != 3) return 2;
    unsigned char request = 0;
    ssize_t count;
    do {
        count = read(300, &request, 1);
    } while (count < 0 && errno == EINTR);
    close(300);
    if (count != 1 || request != 1) return 0;
    // Keep collector status separately: a failed attach must never look like a stack.
    char status[4096];
    std::snprintf(status, sizeof(status), "%s.status", argv[2]);
    const int log = open(status, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    if (log >= 0) {
        posix_spawn_file_actions_adddup2(&actions, log, 1);
        posix_spawn_file_actions_adddup2(&actions, log, 2);
    }
    const int stack_log = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (stack_log >= 0) {
        posix_spawn_file_actions_adddup2(&actions, stack_log, 1);
        posix_spawn_file_actions_adddup2(&actions, stack_log, 2);
    }
    char attach[96];
    std::snprintf(attach, sizeof(attach), "process attach --pid %s", argv[1]);
    char* arguments[]{const_cast<char*>("/usr/bin/lldb"),
                      const_cast<char*>("--batch"),
                      const_cast<char*>("-o"),
                      attach,
                      const_cast<char*>("-o"),
                      const_cast<char*>("thread backtrace all"),
                      const_cast<char*>("-o"),
                      const_cast<char*>("register read"),
                      const_cast<char*>("-o"),
                      const_cast<char*>("image list -o -f"),
                      const_cast<char*>("-o"),
                      const_cast<char*>("process detach"),
                      nullptr};
    pid_t child = -1;
    const int error = posix_spawn(&child, "/usr/bin/lldb", &actions, nullptr, arguments, environ);
    posix_spawn_file_actions_destroy(&actions);
    int state = -1;
    if (!error)
        while (waitpid(child, &state, 0) < 0 && errno == EINTR) {
        }
    if (stack_log >= 0) close(stack_log);
    bool frames = false;
    if (auto* report = std::fopen(argv[2], "rb")) {
        char line[1024];
        while (std::fgets(line, sizeof(line), report))
            if (std::strstr(line, "frame #")) frames = true;
        std::fclose(report);
    }
    const unsigned char result = !error && WIFEXITED(state) && WEXITSTATUS(state) == 0 && frames ? 1 : 2;
    if (log >= 0) {
        dprintf(log, "collector_spawn=%d wait_status=%d success=%d\n", error, state, result == 1);
        close(log);
    }
    (void)write(301, &result, 1);
    close(301);
    return result == 1 ? 0 : 1;
}

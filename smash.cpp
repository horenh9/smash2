#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char *argv[]) {
    if (signal(SIGTSTP, ctrlZHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if (signal(SIGINT, ctrlCHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }
    struct sigaction alarm;
    alarm.sa_handler = alarmHandler;
    alarm.sa_flags = SA_RESTART;
    if (sigaction(SIGALRM, &alarm, nullptr) == -1) {
        perror("smash error: failed to set alarm handler");
    }

    SmallShell &smash = SmallShell::getInstance();
    while (true) {
        cout << smash.prompt_name << "> ";
        string cmd_line;
        getline(cin, cmd_line);
        smash.executeCommand(cmd_line);
    }
    return 0;
}

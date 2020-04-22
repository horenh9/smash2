#include <iostream>
#include <signal.h>
#include <sys/wait.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    SmallShell &shell = SmallShell::getInstance();
    cout << "smash: got ctrl-Z " + to_string(shell.getPidInFG()) << endl;
    kill(shell.getPidInFG(), SIGSTOP);
    shell.jobs->addJob(shell.getCommand(), shell.getPidInFG(), 0);
}

void ctrlCHandler(int sig_num) {
    // TODO: Add your implementation
}

void alarmHandler(int sig_num) {
    // TODO: Add your implementation
}


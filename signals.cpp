#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    SmallShell &shell = SmallShell::getInstance();
    cout << "smash: got ctrl-Z " + to_string(shell.getPidInFG()) << endl;
    kill(shell.getPidInFG(), sig_num);
}

void ctrlCHandler(int sig_num) {
    // TODO: Add your implementation
}

void alarmHandler(int sig_num) {
    // TODO: Add your implementation
}


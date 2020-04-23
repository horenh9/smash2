#include <iostream>
#include <signal.h>
#include <sys/wait.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    SmallShell &smash = SmallShell::getInstance();
    cout << "smash: got ctrl-Z" << endl;
    if (smash.getPidInFG() != -1) {
        kill(smash.getPidInFG(), SIGTSTP);
        if (smash.getJob())
            smash.jobs->addJob(smash.getJob());
        else
            smash.jobs->addJob(smash.getCommand(), smash.getPidInFG(), 0);
        cout << "smash: process " + to_string(smash.getPidInFG()) + " was stopped" << endl;
        smash.setCommand(nullptr);
        smash.setJob(nullptr);
        smash.setPidInFG(-1);
    }
}

void ctrlCHandler(int sig_num) {
    // TODO: Add your implementation
}

void alarmHandler(int sig_num) {
    // TODO: Add your implementation
}


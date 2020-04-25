#include <iostream>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    SmallShell &smash = SmallShell::getInstance();
    string print = "smash: got ctrl-Z\n";
    write_to(print, smash.getOut());
    if (smash.getPidInFG() != -1) {
        kill(smash.getPidInFG(), SIGTSTP);
        if (smash.getJob())
            smash.jobs->addJob(smash.getJob());
        else
            smash.jobs->addJob(smash.getCommand(), smash.getPidInFG(), 0);
        print = "smash: process " + to_string(smash.getPidInFG()) + " was stopped\n";
        write_to(print, smash.getOut());
        smash.setNulls();
    }
}

void ctrlCHandler(int sig_num) {
    SmallShell &smash = SmallShell::getInstance();
    string print = "smash: got ctrl-C\n";
    write_to(print, smash.getOut());
    if (smash.getPidInFG() != -1) {
        kill(smash.getPidInFG(), SIGKILL);
        print = "smash: process " + to_string(smash.getPidInFG()) + " was killed\n";
        write_to(print, smash.getOut());
    }
    smash.setNulls();
}

void alarmHandler(int sig_num) {
    SmallShell &smash = SmallShell::getInstance();
    string print = "smash got an alarm\n";
    write_to(print, smash.getOut());
    if (smash.getPidInFG() != -1) {
        kill(smash.getPidInFG(), SIGKILL);
        cout << "smash: " << smash.getCommand()->getJob() << " timed out!" << endl;
    }
    smash.setNulls();
}


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
    cout << "smash: got ctrl-Z" << endl;
    if (smash.getPidInFG() != -1) {
        if (dynamic_cast<PipeCommand *>(smash.getCommand()))
            killpg(smash.getPidInFG(), SIGTSTP);
        else
            kill(smash.getPidInFG(), SIGTSTP);
        if (smash.getJob())
            smash.jobs->addJob(smash.getJob());
        else
            smash.jobs->addJob(smash.getCommand(), smash.getPidInFG(), 0);
        print = "smash: process " + to_string(smash.getPidInFG()) + " was stopped";
        cout << print << endl;
        smash.setNulls();
    }
}

void ctrlCHandler(int sig_num) {
    SmallShell &smash = SmallShell::getInstance();
    string print = "smash: got ctrl-C";
    cout << print << endl;
    if (smash.getPidInFG() != -1) {
        if (dynamic_cast<PipeCommand *>(smash.getCommand()))
            killpg(smash.getPidInFG(), SIGKILL);
        else
            kill(smash.getPidInFG(), SIGKILL);
        print = "smash: process " + to_string(smash.getPidInFG()) + " was killed";
        cout << print << endl;
    }
    smash.setNulls();
}

void alarmHandler(int sig_num) {
    SmallShell &smash = SmallShell::getInstance();
    string print = "smash got an alarm\n";
    write_to(print, smash.getOut());
    if (smash.getPidInFG() != -1) {
        killpg(smash.getPidInFG(), SIGKILL);
        cout << "smash: " << smash.getCommand()->getJob() << " timed out!" << endl;
    }
    smash.setNulls();
}


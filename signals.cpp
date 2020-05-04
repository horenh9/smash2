#include <iostream>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    bool ispiped = false;
    int countforpipe = 0;
    SmallShell &smash = SmallShell::getInstance();
    string print = "smash: got ctrl-Z";
    cout << print << endl;
    if (smash.getPidInFG() != -1) {
        if (dynamic_cast<PipeCommand *>(smash.getCommand())) {
            killpg(smash.getPidInFG(), SIGSTOP);
            ispiped = true;
            countforpipe = 2;
        } else
            kill(smash.getPidInFG(), SIGSTOP);
        if (smash.getJob())
            smash.jobs->addJob(smash.getJob());
        else
            smash.jobs->addJob(smash.getCommand(), smash.getPidInFG(), 0, ispiped, countforpipe);
        print = "smash: process " + to_string(smash.getPidInFG()) + " was stopped";
        cout << print << endl;
    }
    smash.setNulls();
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
    string print = "smash got an alarm";
    cout << print << endl;
    if (smash.next_alarm_pid != 0) {
        int x = killpg(smash.next_alarm_pid, SIGKILL);
        if (x != -1)
            cout << "smash: " << smash.getTimeCommandByPid(smash.next_alarm_pid) << " timed out!" << endl;
        for (auto it = smash.timeoutlist->begin(); it != smash.timeoutlist->end(); ++it) {
            if (it->pid == smash.next_alarm_pid) {
                smash.timeoutlist->erase(it);
                break;
            }
        }
    }
    smash.ReAlarm();
}
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <fcntl.h>
#include <semaphore.h>
#include <limits.h> /* PATH_MAX */

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cerr << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cerr << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif


/******************** Getters/Setters************/
time_com::time_com(int pid, int duration, time_t start, string command) : pid(pid), duration(duration), start(start),command(command) {}

string *Command::getJob() const {
    return cmd;
}

int JobsList::JobEntry::getMode() const {
    return mode;
}

int JobsList::JobEntry::getJobId() const {
    return jobId;
}

int JobsList::JobEntry::getPid() const {
    return pid;
}

time_t JobsList::JobEntry::getBegin() const {
    return begin;
}

string JobsList::JobEntry::getJob() const {
    return job_name;
}

void JobsList::JobEntry::setMode(int new_mode) {
    mode = new_mode;
}

Command *JobsList::JobEntry::getCommand() {
    return cmd;
}

int SmallShell::getPid() {
    return pid;
}

void SmallShell::setPidInFG(pid_t currpid) {
    curr_pid = currpid;
}

int SmallShell::getPidInFG() {
    return curr_pid;
}

void SmallShell::setJob(JobsList::JobEntry *job) {
    currjob = job;
}

Command *SmallShell::getCommand() {
    return currcmd;
}

void SmallShell::setCommand(Command *cmd) {
    currcmd = cmd;
}

JobsList::JobEntry *SmallShell::getJob() {
    return currjob;
}

int SmallShell::getOut() {
    return out;
}

void SmallShell::setOut(int fd) {
    out = fd;
}

void JobsList::JobEntry::restartTime() {
    begin = time(nullptr);
}


/******************** strings shit ************/

void write_to(string print, int out) {
    const char *p = print.c_str();
    write(out, p, print.size());
}

string _ltrim(const string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == string::npos) ? "" : s.substr(start);
}

string _rtrim(const string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const string &cmd_line, string *args) {
    FUNC_ENTRY()
    int i = 0;
    istringstream iss(_trim(cmd_line));
    for (string s; iss >> s;) {
        args[i] = s;
        args[++i] = "";
    }
    return i;
}

bool _isBackgroundComamnd(const string &cmd_line) {
    const string &str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void arrange_cmd_redirection(string cmd_line, string *actual_cmd, string *file) {
    string temp = _trim(cmd_line);
    if (_isBackgroundComamnd(temp))
        temp = temp.substr(0, temp.length() - 1);
    int pos1 = temp.find_first_of('>');
    int pos2 = temp.find_last_of('>');
    if (pos1 == pos2) {// '>' redirection
        *actual_cmd = _trim(temp.substr(0, pos1));
        *file = _trim(temp.substr(pos1 + 1, cmd_line.length() - pos1 - 1));
    } else {// '>>' redirection
        *actual_cmd = _trim(temp.substr(0, pos1));
        *file = _trim(temp.substr(pos2 + 1, cmd_line.length() - pos2 - 1));
    }
}

void arrange_cmd_pipe(const string &cmd_line, string *cmd1, string *cmd2) {//
    string temp = _rtrim(cmd_line);
    if (_isBackgroundComamnd(temp))
        temp = temp.substr(0, temp.length() - 1);
    int pos1 = temp.find_first_of('|');
    int pos2 = temp.find_last_of('&');
    if (pos2 == int(string::npos)) {
        *cmd1 = _trim(cmd_line.substr(0, pos1));
        *cmd2 = _trim(cmd_line.substr(pos1 + 1, temp.length() - pos1 - 1));
    } else {
        *cmd1 = _trim(cmd_line.substr(0, pos1));
        *cmd2 = _trim(cmd_line.substr(pos2 + 1, temp.length() - pos2 - 1));
    }
}

/******************** Small Shell ************/

SmallShell::SmallShell() : curr_pid(-1), currcmd(nullptr), currjob(nullptr) {
    prompt_name = "smash";
    jobs = new JobsList();
    plastPwd = "";
    pid = getpid();
    timeoutlist = new list<time_com>();
}

SmallShell::~SmallShell() {
    delete jobs;
    delete timeoutlist;
}

Command *SmallShell::CreateCommand(const string &cmd_line, int out, int in, int err) {
    string *cmd = new string[COMMAND_MAX_ARGS];
    for (int i = 0; i < COMMAND_MAX_ARGS; ++i)
        cmd[i] = "";
    int count = _parseCommandLine(cmd_line, cmd);
    string command_name = cmd[0];
    string cmd_s = string(cmd_line);
    delete[] cmd;
    if ("timeout" == command_name)
        return new TimeoutCommand(cmd_line, this, jobs);
    else if (cmd_s.find('|') != string::npos)
        return new PipeCommand(cmd_line, this);
    else if (cmd_s.find('>') != string::npos)
        return new RedirectionCommand(cmd_line, this);
    else if ("chprompt" == command_name || (count == 1 && "chprompt&" == command_name))
        return new ChangePrompt(cmd_line, &prompt_name);
    else if ("pwd" == command_name || "pwd&" == command_name)
        return new GetCurrDirCommand(cmd_line, out, in, err);
    else if ("cd" == command_name)
        return new ChangeDirCommand(cmd_line, &plastPwd, out, in, err);
    else if ("showpid" == command_name || "showpid&" == command_name)
        return new ShowPidCommand(cmd_line, this, out, in, err);
    else if ("jobs" == command_name || "jobs&" == command_name)
        return new JobsCommand(cmd_line, jobs, out, in, err);
    else if ("kill" == command_name)
        return new KillCommand(cmd_line, jobs, out, in, err);
    else if ("fg" == command_name)
        return new ForegroundCommand(cmd_line, jobs, this, out, in, err);
    else if ("bg" == command_name)
        return new BackgroundCommand(cmd_line, jobs, out, in, err);
    else if ("quit" == command_name || "quit&" == command_name)
        return new QuitCommand(cmd_line, jobs, out, in, err);
    else if ("cp" == command_name)
        return new CopyCommand(cmd_line, jobs, this, out, in, err);
    else
        return new ExternalCommand(cmd_line, jobs, this, out, in, err);

}

void SmallShell::executeCommand(const string &cmd_line) {
    Command *cmd = CreateCommand(cmd_line);
    cmd->execute();
}

void SmallShell::setNulls(bool b) {
    setCommand(nullptr);
    setJob(nullptr);
    setPidInFG(-1);
    if (b)
        setOut(1);
}

void SmallShell::ReAlarm() {
    double closest = 0;
    int alarmpid = 0;
    double temp;
    for (auto &it : *timeoutlist) {//find the next alarm
        temp = it.duration - difftime(time(nullptr), it.start);
        if (temp < closest || closest == 0) {
            closest = temp;
            alarmpid = it.pid;
        }
    }
    if (alarmpid != 0) {
        next_alarm_pid = alarmpid;
        alarm(closest);
    } else
        next_alarm_pid = 0;
}

string SmallShell::getTimeCommandByPid(pid_t pid){
    for (auto it = timeoutlist->begin(); it != timeoutlist->end(); ++it)
        if (it->pid == pid)
            return it->command;
}
/******************** Jobs ************/

JobsList::JobsList() : max(0) {
    jobs_list = new list<JobEntry>();
}

JobsList::~JobsList() {
    delete jobs_list;
}

JobsList::JobEntry::JobEntry(string *job, int jobId, int pid, int mode, Command *cmd) :
        jobId(jobId), pid(pid), mode(mode), begin(time(nullptr)), cmd(cmd) {
    isPipe = false;
    job_name = cmd->cmd_line;
}


int JobsList::addJob(Command *cmd, int pid, int mode, bool isPiped, int counter) {
    removeFinishedJobs();
    JobEntry job = JobEntry(cmd->getJob(), max + 1, pid, mode, cmd);
    job.counter = counter;
    job.isPipe = isPiped;
    jobs_list->push_back(job);
    return job.getJobId();
}

void JobsList::printJobsList(int out) {
    removeFinishedJobs();
    for (auto &it : *jobs_list) {
        double diff = difftime(time(nullptr), it.getBegin());
        string mode = "";
        if (it.getMode() == 0)
            mode = "(stopped)";
        string print = "[" + to_string(it.getJobId()) + "] " + it.getJob() + " : " + to_string(it.getPid()) + " " +
                       to_string(int(diff)) + " secs " + mode + "\n";
        write_to(print, out);
    }
}

void JobsList::killAllJobs(int out) {
    for (auto it = jobs_list->begin(); it != jobs_list->end(); ++it) {
        string print = to_string(it->getPid()) + ": " + it->getJob() + "\n";
        write_to(print, out);
        if (it->isPipe)
            killpg(-9, it->getJobId());
        else
            kill(-9, it->getJobId());
        ++it;
        removeJobById((--it)->getJobId());
        --it;
    }
}

void JobsList::removeFinishedJobs() {
    int currMax = 0, check = 0;
    for (auto it = jobs_list->begin(); it != jobs_list->end(); ++it) {
        if (it->isPipe) {

            for (;;) {
                siginfo_t status = {0};
                waitid(P_PGID, it->getPid(), &status, WNOHANG | WEXITED);
                if (status.si_pid == 0) break;

                it->counter--;
            }
            if (it->counter == 0) {
                ++it;
                removeJobById((--it)->getJobId());
                --it;
            }
        } else if (waitpid(it->getPid(), &check, WNOHANG) != 0) {
            ++it;
            removeJobById((--it)->getJobId());
            --it;
        }
        if (it->getJobId() > currMax)
            currMax = it->getJobId();

        check = 0;
    }
    max = currMax;
}

JobsList::JobEntry *JobsList::getJobById(int jobId) {
    for (auto &it : *jobs_list)
        if (it.getJobId() == jobId)
            return &it;
    return nullptr;
}

void JobsList::removeJobById(int jobId) {
    for (auto it = jobs_list->begin(); it != jobs_list->end(); ++it)
        if (it->getJobId() == jobId) {
            jobs_list->erase(it);
            return;
        }
}

JobsList::JobEntry *JobsList::getLastJob(int *lastJobId) {
    (*lastJobId) = max;
    for (auto &it : *jobs_list)
        if (it.getJobId() == max)
            return &it;
    return nullptr;
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId) {
    JobEntry *maxStopped = &(*(jobs_list->begin()));
    (*jobId) = -1;
    for (auto &it : *jobs_list)
        if (it.getJobId() > maxStopped->getJobId() && it.getMode() == 0)
            maxStopped = &it;
    if (maxStopped != nullptr) {
        if (maxStopped->getMode() != 0)
            return nullptr;
        (*jobId) = maxStopped->getJobId();
    }
    return maxStopped;
}

void JobsList::addJob(JobEntry *job) {
    for (auto it = jobs_list->begin(); it != jobs_list->end(); ++it) {
        if (it->getJobId() < job->getJobId() && (it++) != jobs_list->end() && it->getJobId() > job->getJobId()) {
            job->setMode(0);
            job->restartTime();
            jobs_list->insert(it, *job);
            return;
        }
        it--;
    }
}

/******************** Command Constructors & Destructors************/

Command::Command(const string &cmd_line, int out, int in, int err) : out(out), in(in), err(err), cmd_line(cmd_line) {
    cmd = new string[COMMAND_MAX_ARGS];
    for (int i = 0; i < COMMAND_MAX_ARGS; ++i)
        cmd[i] = "";
    _parseCommandLine(cmd_line, cmd);
    // bg = _isBackgroundComamnd(cmd_line);
}

Command::~Command() {
    delete[] cmd;
}

ExternalCommand::ExternalCommand(const string &cmd_line, JobsList *jobs, SmallShell *smash, int out, int in, int err,
                                 bool piped) : Command(cmd_line, out, in, err), jobs(jobs), smash(smash),
                                               isPiped(piped), extCommFromRed(nullptr) {}

RedirectionCommand::RedirectionCommand(const string &cmd_line, SmallShell *smash) : Command(cmd_line), smash(smash) {
}

PipeCommand::PipeCommand(const string &cmd_line, SmallShell *smash) :
        Command(cmd_line), smash(smash), cmd1(nullptr), cmd2(nullptr) {
    if (cmd_line.find("|&") != string::npos)
        op = "|&";
    else
        op = "|";
    arrange_cmd_pipe(cmd_line, &command1, &command2);
}

PipeCommand::~PipeCommand() {
    delete cmd1;
    delete cmd2;
}

TimeoutCommand::TimeoutCommand(const string &cmd_line, SmallShell *smash, JobsList *jobs) :
        Command(cmd_line), smash(smash), jobs(jobs) {
    try {
        duration = stoi(cmd[1]);
    } catch (...) {
        string print = "smash error: timeout: invalid arguments\n";
        write_to(print, out);
        return;
    }
    if (duration <= 0 || cmd[2].empty()) {
        string print = "smash error: timeout: invalid arguments\n";
        write_to(print, out);
        return;
    }
    string comm;
    int counter = 2;
    while (!cmd[counter].empty()) {
        comm += cmd[counter] + " ";
        counter++;
    }
    comm = _trim(comm);
    if (_isBackgroundComamnd(comm))
        comm = cmd_line.substr(0, cmd_line.size() - 1);
    actual_cmd = smash->CreateCommand(comm);
}


/******************** Built in command constructors************/

BuiltInCommand::BuiltInCommand(const string &cmd_line, int out, int in, int err) : Command(cmd_line, out, in, err) {
}

ChangePrompt::ChangePrompt(const string &cmd_line, string *prompt_name) :
        BuiltInCommand(cmd_line), prompt(prompt_name) {
}

JobsCommand::JobsCommand(const string &cmd_line, JobsList *jobs, int out, int in, int err) :
        BuiltInCommand(cmd_line, out, in, err), jobs(jobs) {
}

ChangeDirCommand::ChangeDirCommand(const string &cmd_line, string *plastPwd, int out, int in, int err) :
        BuiltInCommand(cmd_line, out, in, err) {
    OLDPWD = plastPwd;
}

KillCommand::KillCommand(const string &cmd_line, JobsList *jobs, int out, int in, int err) :
        BuiltInCommand(cmd_line, out, in, err), jobs(jobs) {
}

ForegroundCommand::ForegroundCommand(const string &cmd_line, JobsList *jobs, SmallShell *smash, int out, int in,
                                     int err)
        : BuiltInCommand(cmd_line, out, in, err), jobs(jobs), smash(smash) {
}

BackgroundCommand::BackgroundCommand(const string &cmd_line, JobsList *jobs, int out, int in, int err) :
        BuiltInCommand(cmd_line, out, in, err), jobs(jobs) {
}

GetCurrDirCommand::GetCurrDirCommand(const string &cmd_line, int out, int in, int err) :
        BuiltInCommand(cmd_line, out, in, err) {
}

ShowPidCommand::ShowPidCommand(const string &cmd_line, SmallShell *smash, int out, int in, int err) :
        BuiltInCommand(cmd_line, out, in, err), smash(smash) {
}

QuitCommand::QuitCommand(const string &cmd_line, JobsList *jobs, int out, int in, int err) :
        BuiltInCommand(cmd_line, out, in, err), jobs(jobs) {
}

CopyCommand::CopyCommand(const string &cmd_line, JobsList *jobs, SmallShell *smash, int out, int in, int err) :
        BuiltInCommand(cmd_line, out, in, err), jobs(jobs), smash(smash) {
    path1 = cmd[1];
    if (_isBackgroundComamnd(cmd[2]))
        path2 = cmd[2].substr(0, cmd[2].length() - 1);
    else
        path2 = cmd[2];
}


/******************** Built in Command executes************/

void ChangePrompt::execute() {
    if (cmd[1].empty() || cmd[1] == "&")
        *prompt = "smash";
    else {
        if (_isBackgroundComamnd(cmd[1]))
            *prompt = cmd[1].substr(0, cmd[1].length() - 1);
        else
            *prompt = cmd[1];
    }
}

void ShowPidCommand::execute() {
    string print = "smash pid is " + to_string(smash->getPid()) + "\n";
    write_to(print, out);
}

void GetCurrDirCommand::execute() {
    char buf[COMMAND_ARGS_MAX_LENGTH];
    string print = getcwd(buf, COMMAND_ARGS_MAX_LENGTH);
    if (print.c_str() == nullptr) {
        perror("smash error: getcwd faild");
        return;
    }
    print += "\n";
    write_to(print, out);
}

void ChangeDirCommand::execute() {
    if (cmd[1].empty())
        return;
    if (!cmd[2].empty()) {
        string print = "smash error: cd: too many arguments\n";
        write_to(print, err);
        return;
    }
    string temp = cmd[1]; // load path to temp

    if (temp == "-") {
        if (OLDPWD->empty()) {
            string print = "smash error: cd: OLDPWD not set\n";
            write_to(print, err);
            return;
        }
        temp = *OLDPWD; // if argument is '-' load OLDPWD to temp
    }
    if (_isBackgroundComamnd(temp))
        temp = temp.substr(0, temp.size() - 1);
    char *buf = new char[COMMAND_ARGS_MAX_LENGTH];
    if (getcwd(buf, COMMAND_ARGS_MAX_LENGTH) == nullptr) {
        perror("smash error: getcwd failed");
    } else {
        if (chdir(temp.c_str()) == -1)        //go to wanted directory via syscall
            perror("smash error: chdir failed");
        else
            *OLDPWD = string(buf);
    }
    delete[]buf;
}

void JobsCommand::execute() {
    jobs->printJobsList(out);
}

void KillCommand::execute() {
    jobs->removeFinishedJobs();
    try {
        int id = stoi(cmd[2]);
        if (jobs->getJobById(id) == nullptr) {
            string print = "smash error: kill: job-id " + cmd[2] + " does not exist\n";
            write_to(print, out);
            return;
        }
        int sigNum = stoi(cmd[1].substr(1, cmd[1].length() - 1));
        if (cmd[1].at(0) != '-' || !cmd[3].empty()) {
            string print = "smash error: kill: invalid arguments\n";
            write_to(print, out);
            return;
        }

        int c;
        if (jobs->getJobById(id)->isPipe)
            c = killpg(jobs->getJobById(id)->getPid(), stoi(cmd[1].substr(1, cmd[1].length() - 1)));
        else
            c = kill(jobs->getJobById(id)->getPid(), stoi(cmd[1].substr(1, cmd[1].length() - 1)));
        if (c == -1)
            perror("smash error: kill failed");
        else {
            string print = "signal number " + to_string(sigNum) + " was sent to pid "
                           + to_string(jobs->getJobById(id)->getPid()) + "\n";
            write_to(print, out);
        }

    }
    catch (...) {
        string print = "smash error: kill: invalid arguments\n";
        write_to(print, out);
    }
}

void ForegroundCommand::execute() {
    jobs->removeFinishedJobs();
    int check;
    if (jobs->jobs_list->empty() && cmd[1].empty()) {
        string print = "smash error: fg: jobs list is empty\n";
        write_to(print, err);
        return;
    }
    try {
        int id;
        jobs->getLastJob(&id);
        if (!cmd[1].empty())
            id = stoi(cmd[1]);
        JobsList::JobEntry *job = jobs->getJobById(id);
        if (!cmd[2].empty()) {
            string print = "smash error: fg: invalid arguments\n";
            write_to(print, err);
            return;
        }
        if (job) {
            job->setMode(2);
            string print = job->getJob() + " : " + to_string(job->getPid()) + "\n";
            jobs->removeJobById(id);
            write(out, print.c_str(), print.size());

            smash->setJob(job);
            smash->setCommand(job->getCommand());
            smash->setPidInFG(job->getPid());
            pid_t extpid = fork();
            if (extpid == 0) {//son
                setpgrp();
                dup2(in, 0);
                dup2(out, 1);
                dup2(err, 2);
                if (job->isPipe)
                    killpg(job->getPid(), SIGCONT);
                else
                    kill(job->getPid(), SIGCONT);
                exit(2);

            } else {
                waitpid(job->getPid(), &check, WUNTRACED);
                smash->setNulls();
            }

        } else {
            string print = "smash error: fg: job-id " + to_string(id) + " does not exist\n";
            write_to(print, err);
        }
    }
    catch (...) {
        string print = "smash error: fg: invalid arguments\n";
        write_to(print, err);
    }
}

void BackgroundCommand::execute() {
    jobs->removeFinishedJobs();
    int id;
    JobsList::JobEntry *job = jobs->getLastStoppedJob(&id);
    if (!job && cmd[1].empty()) {
        string print = "smash error: bg: there is no stopped jobs to resume\n";
        write_to(print, err);
        return;
    }
    try {
        if (!cmd[1].empty()) {
            id = stoi(cmd[1]);
            job = jobs->getJobById(id);
        }
        if (!job) {
            string print = "smash error: bg: job-id " + to_string(id) + " does not exist\n";
            write_to(print, err);
            return;
        }
        if (job->getMode() == 1) {
            string print = "smash error: bg: job-id " + to_string(id) + " is already running in the background\n";
            write_to(print, err);
            return;
        }
        if (!cmd[2].empty()) { // there is more than one argument
            string print = "smash error: fg: invalid arguments\n";
            write_to(print, err);
            return;
        }
        job->setMode(1);
        string print = job->getJob() + " : " + to_string(job->getPid()) + "\n";
        write_to(print, out);
        if (job->isPipe)
            killpg(job->getPid(), SIGCONT);
        else
            kill(job->getPid(), SIGCONT);
    }
    catch (...) {
        string print = "smash error: bg: invalid arguments\n";
        write_to(print, err);
    }
}

void QuitCommand::execute() {
    jobs->removeFinishedJobs();
    if (cmd[1] == "kill" || cmd[1] == "kill&") {
        int count = jobs->jobs_list->size();
        string print = "smash: sending SIGKILL signal to " + to_string(count) + " jobs:\n";
        write_to(print, out);
        jobs->killAllJobs(out);
    }
    exit(2);
}

void CopyCommand::execute() {
    char buffer1[PATH_MAX];
    char buffer2[PATH_MAX];
    realpath(cmd[1].c_str(), buffer1);
    realpath(cmd[2].c_str(), buffer2);
    int src = open(path1.c_str(), O_RDONLY);
    if (src == -1) {
        perror("smash error: open failed");
        return;
    }

    if (strcmp(buffer1, buffer2) == 0) {
        close(src);
        string print = "smash: " + path1 + " was copied to " + path2 + "\n";
        write(out, print.c_str(), print.size());
        return;
    }
    int dst = open(path2.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (dst == -1) {
        close(src);
        perror("smash error: open failed");
        return;
    }
    bool bg = _isBackgroundComamnd(cmd_line);
    smash->setCommand(this);

    pid_t cppid = fork();
    smash->setPidInFG(cppid);
    string temp;
    if (bg) {
        smash->setNulls();
        jobs->addJob(this, cppid, 1);
        temp = cmd_line.substr(0, cmd_line.size() - 1);
        if (_isBackgroundComamnd(path2))
            path2 = path2.substr(0, path2.length() - 1);
    } else
        temp = cmd_line;


    if (cppid == 0) {// son
        setpgrp();
        char *buf = new char[SIZE_TO_READ];
        int bytesRead = read(src, buf, SIZE_TO_READ);
        if (bytesRead == -1) {
            perror("smash error: read failed");
            close(src);
            close(dst);
            exit(2);
        }
        while (bytesRead != 0) {
            if (write(dst, buf, bytesRead) == -1) {
                perror("smash error: write failed");
                close(src);
                close(dst);
                exit(2);
            }
            bytesRead = read(src, buf, SIZE_TO_READ);
            if (bytesRead == -1) {
                perror("smash error: read failed");
                close(src);
                close(dst);
                exit(2);
            }
        }
        delete[] buf;
        string print = "smash: " + path1 + " was copied to " + path2 + "\n";
        write(out, print.c_str(), print.size());
        close(src);
        close(dst);
        exit(2);
    } else if (!bg) { //father
        waitpid(cppid, nullptr, WUNTRACED);
        smash->setNulls();
        close(src);
        close(dst);
    }
}

/******************** executes ************/

void RedirectionCommand::execute() {
    int fd;
    string actual_cmd;
    string file_name;
    arrange_cmd_redirection(cmd_line, &actual_cmd, &file_name);
    if (cmd_line.find(">>") != string::npos)
        fd = open(file_name.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);
    else
        fd = open(file_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("smash error: open failed");
        return;
    }

    Command *comm = smash->CreateCommand(actual_cmd, fd);
    smash->setOut(fd);
    auto *ext = dynamic_cast<ExternalCommand *> (comm);
    if (ext != nullptr) {
        ext->extCommFromRed = this;
        ext->execute();
    } else
        comm->execute();
    fflush(stdout);
    if (close(fd) == -1)
        perror("smash error: close failed");
    smash->setOut(1);
}

void ExternalCommand::execute() {
    char *c = (char *) "-c";
    bool bg = _isBackgroundComamnd(cmd_line);
    if (extCommFromRed != nullptr && _isBackgroundComamnd(extCommFromRed->cmd_line))
        bg = true;
    smash->setCommand(this);

    pid_t extpid = fork();
    smash->setPidInFG(extpid);
    string temp;
    if (bg) {
        smash->setNulls(false);
        if (extCommFromRed == nullptr) {
            jobs->addJob(this, extpid, 1);
            temp = cmd_line.substr(0, cmd_line.size() - 1);
        } else {
            jobs->addJob(extCommFromRed, extpid, 1);
            temp = cmd_line;
        }
    } else
        temp = cmd_line;

    if (extpid == 0) {//son
        if (!isPiped)
            setpgrp();
        dup2(in, 0);
        dup2(out, 1);
        dup2(err, 2);
        char *argv[] = {(char *) "/bin/bash", c, (char *) temp.c_str(), nullptr};
        execv("/bin/bash", argv);

    } else if (!bg) { //father
        waitpid(extpid, nullptr, WUNTRACED);
        smash->setNulls();
    }
}

void PipeCommand::execute() {
    int fd[2];
    if (pipe(fd) == -1) {
        string print = "smash error: pipe failed";
        write(err, "smash error: pipe failed", print.size());
        return;
    }
    if (op == "|")
        cmd1 = smash->CreateCommand(command1, fd[1]);
    else
        cmd1 = smash->CreateCommand(command1, out, in, fd[1]);
    cmd2 = smash->CreateCommand(command2, out, fd[0]);
    ExternalCommand *ext1 = dynamic_cast<ExternalCommand *>(cmd1);
    ExternalCommand *ext2 = dynamic_cast<ExternalCommand *>(cmd2);
    if (ext1)
        ext1->isPiped = true;
    if (ext2)
        ext2->isPiped = true;
    smash->setCommand(this);
    pid_t pid1 = fork();
    if (pid1 == 0) { //son
        setpgid(pid1, 0);
        close(fd[0]);
        cmd1->execute();
        close(fd[1]);
        exit(2);
    } else { //father
        close(fd[1]);
        pid_t pid2 = fork();
        if (pid2 == 0) {
            setpgid(pid2, pid1);
            cmd2->execute();
            close(fd[0]);
            exit(2);
        } else {
            close(fd[0]);
            if (!_isBackgroundComamnd(cmd_line)) {
                smash->setPidInFG(pid1);
                waitpid(pid1, nullptr, WUNTRACED);
                waitpid(pid2, nullptr, WUNTRACED);
                smash->setNulls();
            } else {
                smash->jobs->addJob(this, pid1, 1, true, 2);
                smash->setNulls();
            }
        }
    }
}

void TimeoutCommand::execute() {
    bool bg = _isBackgroundComamnd(cmd_line);
    smash->setCommand(this);
    int tmpid = fork();
    time_com commdata = time_com(tmpid, duration, time(nullptr),cmd_line);
    smash->timeoutlist->push_back(commdata);
    smash->ReAlarm();
    smash->setPidInFG(tmpid);
    if (bg) {
        smash->setNulls(false);
        jobs->addJob(this, tmpid, 1);
    }
    ExternalCommand *ext = dynamic_cast<ExternalCommand *>(actual_cmd);
    if (ext)
        ext->isPiped = true;

    if (tmpid == 0) {//son
        setpgid(tmpid, 0);
        actual_cmd->execute();
        exit(2);
    } else if (!bg) { //father
        waitpid(tmpid, nullptr, WUNTRACED);
        smash->setNulls();
    }
}

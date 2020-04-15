#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

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

#define DEBUG_PRINT cerr << "DEBUG: "

#define EXEC(path, arg) \
  execvp((path), (arg));

/******************** strings shit ************/

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const string &cmd_line, string *args) {
    FUNC_ENTRY()
    int i = 0;
    istringstream iss(_trim(cmd_line));
    for (string s; iss >> s;) {
        args[i] = s;
        args[++i] = nullptr;
    }
    return i;
}

bool _isBackgroundComamnd(const string cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(string cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}



// TODO: Add your implementation for classes in Commands.h

SmallShell::SmallShell() {
// TODO: add your implementation
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const string cmd_line) {
    // For example:
/*
  string cmd_s = string(cmd_line);
  if (cmd_s.find("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if ...
  .....
  else {
    return new ExternalCommand(cmd_line);
  }
  */
    return nullptr;
}

void SmallShell::executeCommand(const string cmd_line) {
    // TODO: Add your implementation here
    // for example:
    // Command* cmd = CreateCommand(cmd_line);
    // cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}

/********************************/

Command::Command(const string cmd_line) {
    cmd = new string[COMMAND_MAX_ARGS];
    for (int i = 0; i < COMMAND_MAX_ARGS; ++i)
        cmd[i] = "";
    _parseCommandLine(cmd_line, cmd);
    bg = _isBackgroundComamnd(cmd_line);
}

Command::~Command() {
    delete[] cmd;
}

/******************** Built in command constructors************/

BuiltInCommand::BuiltInCommand(const string cmd_line) : Command(cmd_line) {
}

JobsCommand::JobsCommand(const string cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {
}

ChangeDirCommand::ChangeDirCommand(const string cmd_line, string *plastPwd) : BuiltInCommand(cmd_line) {
    OLDPWD = *plastPwd;
}

KillCommand::KillCommand(const string cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {
}

ForegroundCommand::ForegroundCommand(const string cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {
}

BackgroundCommand::BackgroundCommand(const string cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {
}

GetCurrDirCommand::GetCurrDirCommand(const string cmd_line) : BuiltInCommand(cmd_line) {
}

ShowPidCommand::ShowPidCommand(const string cmd_line) : BuiltInCommand(cmd_line) {
}

/******************** Jobs ************/

JobsList::JobsList() : max(0) {
    jobs = new list<JobEntry>();
}

JobsList::~JobsList() {
    delete jobs;
}

JobsList::JobEntry::JobEntry(string *job, int jobId, int pid, int mode) : jobId(jobId), pid(pid), mode(mode),
                                                                          begin(time(0)) {
    int i = 0;
    job_name = "";
    while (job[i] != "")
        job_name += job[i] + " ";
    job_name.substr(job_name.size(), 1);
}

void JobsList::addJob(Command *cmd, bool isStopped) {
    removeFinishedJobs();
    JobEntry job = JobEntry(cmd->getJob(), max + 1, cmd->getPid(), !isStopped);
    jobs->push_back(job);
}

void JobsList::printJobsList() {
    removeFinishedJobs();
    for (auto it = jobs->begin(); it != jobs->end(); ++it) {
        double diff = difftime(it->getBegin(), time(0));
        string mode = "";
        if (it->getMode() == 0)
            mode = "(stopped)";
        cout << '[' << it->getJobId() << "] " << it->getJob() << " : " << it->getPid() << " " << (int) diff << " secs "
             << mode << endl;
    }
}

void JobsList::killAllJobs() {
    for (auto it = jobs->begin(); it != jobs->end(); ++it) {
        KillCommand killcmd = KillCommand(it->getJob(), this);
        killcmd.execute();
        removeJobById(it->getJobId());
    }
}

void JobsList::removeFinishedJobs() {
    int currMax = 0;
    for (auto it = jobs->begin(); it != jobs->end(); ++it) {
        if (it->getMode() > 1)
            removeJobById(it->getJobId());
        else if (it->getJobId() > currMax)
            currMax = it->getJobId();
    }
    max = currMax;
}

JobsList::JobEntry *JobsList::getJobById(int jobId) {
    for (auto it = jobs->begin(); it != jobs->end(); ++it)
        if (it->getJobId() == jobId)
            return &(*it);
    return nullptr;
}

void JobsList::removeJobById(int jobId) {
    for (auto it = jobs->begin(); it != jobs->end(); ++it)
        if (it->getJobId() == jobId)
            jobs->remove(*it);
}

JobsList::JobEntry *JobsList::getLastJob(int *lastJobId) {
    (*lastJobId) = max;
    for (auto it = jobs->begin(); it != jobs->end(); ++it)
        if (it->getJobId() == max)
            return &(*it);
    return nullptr;
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId) {
    JobEntry *maxStopped = &(*(jobs->begin()));
    (*jobId) = 0;
    for (auto it = jobs->begin(); it != jobs->end(); ++it)
        if (it->getJobId() > maxStopped->getJobId() && it->getMode() == 0)
            maxStopped = &(*it);
    if (maxStopped != nullptr)
        (*jobId) = maxStopped->getJobId();
    return maxStopped;
}

/******************** Getters/Setters************/


bool Command::getBg() const {
    return bg;
}

int Command::getPid() const {
    return pid;
}

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

void JobsList::JobEntry::setMode(int mode) {
    JobEntry::mode = mode;
}

/******************** Built in command executes************/

void ShowPidCommand::execute() {
    cout << "smash pid is " << getPid() << endl;
}

void GetCurrDirCommand::execute() {
    char *buf = new char[COMMAND_ARGS_MAX_LENGTH];
    getcwd(buf, COMMAND_ARGS_MAX_LENGTH);
    cout << buf << endl;
    delete[] buf;
}

void ChangeDirCommand::execute() {
    if (cmd[2].empty()) {
        cerr << "smash error: cd: too many arguments" << endl;
        return;
    }
    string temp = cmd[1]; // load path to temp

    if (temp == "-") {
        if (OLDPWD.empty()) {
            cerr << "smash error: cd: OLDPWD not set" << endl;
            return;
        }
        temp = OLDPWD; // if argument is '-' load OLDPWD to temp
    }
    char *buf = new char[COMMAND_ARGS_MAX_LENGTH];
    getcwd(buf, COMMAND_ARGS_MAX_LENGTH);
    if (chdir(temp.data()) == -1)        //go to wanted directory via syscall
        perror("smash error: chdir failed");
    else
        OLDPWD = string(buf);

    delete[]buf;
}

void JobsCommand::execute() {
    jobs->printJobsList();
}

void KillCommand::execute() {
    jobs->removeFinishedJobs();
    int id = stoi(cmd[2]);
    if (jobs->getJobById(int(cmd[2])) == nullptr) {
        cerr << "smash error: kill: job-id " << cmd[2] << " does not exist" << endl;
        return;
    }
    if (cmd[1][0] != '-' || cmd[3] != "") {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    cout << "signal number " << cmd[1] << " was sent to pid " << jobs->getJobById(int(cmd[2]))->getPid() << endl;
    kill(jobs->getJobById(int(cmd[2]))->getPid(), int(cmd[1]));
}

void ForegroundCommand::execute() {
    jobs->removeFinishedJobs();
    int id;
    JobsList::JobEntry *job = jobs->getLastJob(&id);
    if (cmd[1])
        id = int(cmd[1]);
    job = jobs->getJobById(id);

    if (job) {
        job->setMode(2);
        jobs->removeJobById(id);
        cout << job->getJob() << " : " << job->getPid() << endl;
        kill(job->getPid(), SIGCONT);
        waitpid(job->getPid(), nullptr, 0);

    } else
        cerr << "smash error: fg: job-id " << id << " does not exist" << endl;
}

void BackgroundCommand::execute() {
    int id;
    jobs->removeFinishedJobs();
    JobsList::JobEntry *job = jobs->getLastStoppedJob(&id);
    if (cmd[1] == "") {
        id = int(cmd[1]);
        job = jobs->getJobById(int(cmd[1]));
    }
    if (job) {
        job->setMode(1);
        cout << job->getJob() << " : " << job->getPid() << endl;
        kill(job->getPid(), SIGCONT);
    } else
        cerr << "smash error: fg: job-id " << id << " does not exist" << endl;
}




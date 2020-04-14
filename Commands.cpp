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

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
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



/********************************/


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
Command *SmallShell::CreateCommand(const char *cmd_line) {
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

void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    // for example:
    // Command* cmd = CreateCommand(cmd_line);
    // cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}

Command::Command(const char *cmd_line) {
    _parseCommandLine(cmd_line, cmd);
    bg = _isBackgroundComamnd(cmd_line);
}

/******************** Jobs ************/


JobsList::JobsList() : max(0) {
    jobs = new list<JobEntry>();
}

JobsList::~JobsList() {
    delete jobs;
}

JobsList::JobEntry::JobEntry(string job, int jobId, int pid, int mode) : job(job), jobId(jobId), pid(pid), mode(mode),
                                                                         begin(time(0)) {
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
    jobs->clear();//maybe there are leaks here, need to make sure.
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
            return &(*it); // weird as hell, need to make sure that it is legit
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

string Command::getJob() const {
    return cmd[0];//not sure in which position yet
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

const string JobsList::JobEntry::getJob() const {
    return job;
}

void JobsList::JobEntry::setMode(int mode) {
    JobEntry::mode = mode;
}

void JobsList::JobEntry::setJobId(int jobId) {
    JobEntry::jobId = jobId;
}


/******************** Built in command************/

void ShowPidCommand::execute() {
    cout << "smash pid is " << getPid() << endl;
}

void GetCurrDirCommand::execute() {
    char *buf = getcwd(buf, 100);
    cout << buf << endl;
}


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


SmallShell::SmallShell() {
    prompt_name = "smash";
    jobs = new JobsList();
    plastPwd = "";
}

SmallShell::~SmallShell() {
    delete jobs;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const string cmd_line) {

    string cmd_s = string(cmd_line);
    if (cmd_s.find(">") == 0)
        return new RedirectionCommand(cmd_line, this);
    else if (cmd_s.find("chprompt") == 0)
        return new ChangePrompt(cmd_line, &prompt_name);
    else if (cmd_s.find("pwd") == 0)
        return new GetCurrDirCommand(cmd_line);
    else if (cmd_s.find("cd") == 0)
        return new ChangeDirCommand(cmd_line, &plastPwd);
    else if (cmd_s.find("showpid") == 0)
        return new ShowPidCommand(cmd_line);
    else if (cmd_s.find("jobs") == 0)
        return new JobsCommand(cmd_line, jobs);
    else if (cmd_s.find("kill") == 0)
        return new KillCommand(cmd_line, jobs);
    else if (cmd_s.find("fg") == 0)
        return new ForegroundCommand(cmd_line, jobs);
    else if (cmd_s.find("bg") == 0)
        return new BackgroundCommand(cmd_line, jobs);
    else if (cmd_s.find("quit") == 0)
        return new QuitCommand(cmd_line, jobs);
}

void SmallShell::executeCommand(const string cmd_line) {
    // TODO: Add your implementation here
    // for example:
    // Command* cmd = CreateCommand(cmd_line);
    // cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
    Command *cmd = CreateCommand(cmd_line);
    cmd->execute();
}

/********************************/

Command::Command(const string cmd_line) : cmd_line(cmd_line) {
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

ExternalCommand::ExternalCommand(const string cmd_line, JobsList *jobs) : Command(cmd_line), jobs(jobs) {
}


BuiltInCommand::BuiltInCommand(const string cmd_line) : Command(cmd_line) {
}


ChangePrompt::ChangePrompt(const string cmd_line, string *prompt_name) : BuiltInCommand(cmd_line), prompt(prompt_name) {
}

JobsCommand::JobsCommand(const string cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {
}

ChangeDirCommand::ChangeDirCommand(const string cmd_line, string *plastPwd) : BuiltInCommand(cmd_line) {
    OLDPWD = plastPwd;
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

QuitCommand::QuitCommand(const string cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {
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
        cout << '[' << it->getJobId() << "] " << it->getJob() << " : " << it->getPid() << " " << (int) diff
             << " secs "
             << mode << endl;
    }
}

void JobsList::killAllJobs() {
    for (auto it = jobs->begin(); it != jobs->end(); ++it) {
        cout << it->getPid() << ": " << it->getJob() << endl;
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

void ExternalCommand::execute() {
    pid_t p = fork();
    if (p == 0) {
        char *c = "-c";
        if (cmd_line.at(cmd_line.size()) == '&')
            jobs->addJob(this);
        const char *argv[] = {c, cmd_line.c_str(), nullptr};
        execv("/bin/bash", argv);
    } else {
        wait(nullptr);
    }
}


void ChangePrompt::execute() {
    if (cmd[1].empty())
        *prompt = "smash";
    else
        *prompt = cmd[1];
}

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
        if (OLDPWD->empty()) {
            cerr << "smash error: cd: OLDPWD not set" << endl;
            return;
        }
        temp = *OLDPWD; // if argument is '-' load OLDPWD to temp
    }
    char *buf = new char[COMMAND_ARGS_MAX_LENGTH];
    getcwd(buf, COMMAND_ARGS_MAX_LENGTH);
    if (chdir(temp.data()) == -1)        //go to wanted directory via syscall
        perror("smash error: chdir failed");
    else
        *OLDPWD = string(buf);

    delete[]buf;
}

void JobsCommand::execute() {
    jobs->printJobsList();
}

void KillCommand::execute() {
    jobs->removeFinishedJobs();
    try {
        int id = stoi(cmd[2]);
        if (jobs->getJobById(id) == nullptr) {
            cerr << "smash error: kill: job-id " << cmd[2] << " does not exist" << endl;
            return;
        }
        if (cmd[1].at(0) != '-' || !cmd[3].empty()) {
            cerr << "smash error: kill: invalid arguments" << endl;
            return;
        }
        cout << "signal number " << cmd[1].substr(1, cmd[1].length() - 1) << " was sent to pid "
             << jobs->getJobById(id)->getPid() << endl;
        kill(jobs->getJobById(id)->getPid(), stoi(cmd[1].substr(1, cmd[1].length() - 1))));
    }
    catch (...) {
        cerr << "smash error: kill: invalid arguments" << endl;
    }
}

void ForegroundCommand::execute() {
    jobs->removeFinishedJobs();
    if (jobs->jobs->empty() && cmd[1].empty()) {
        cerr << "smash error: fg: jobs list is empty " << endl;
        return;
    }
    try {
        int id = jobs->getLastJob(&id)->getJobId();
        if (!cmd[1].empty())
            id = stoi(cmd[1]);
        JobsList::JobEntry *job = jobs->getJobById(id);

        if (job) {
            job->setMode(2);
            jobs->removeJobById(id);
            cout << job->getJob() << " : " << job->getPid() << endl;
            kill(job->getPid(), SIGCONT);
            waitpid(job->getPid(), nullptr, 0);

        } else
            cerr << "smash error: fg: job-id " << id << " does not exist" << endl;
    }
    catch (...) {
        cerr << "smash error: fg: invalid arguments" << endl;
    }
}

void BackgroundCommand::execute() {
    jobs->removeFinishedJobs();
    int id;
    JobsList::JobEntry *job = jobs->getLastStoppedJob(&id);
    if (!job && cmd[1].empty()) {
        cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
        return;
    }
    try {
        if (!cmd[1].empty()) {
            id = stoi(cmd[1]);
            job = jobs->getJobById(id);
        }
        if (!job) {
            cerr << "smash error: bg: job-id " << id << " does not exist" << endl;
            return;
        }
        if (job->getMode() == 1) {
            cerr << " smash error: bg: job-id" << id << "is already running in the background" << endl;
            return;
        }
        job->setMode(1);
        cout << job->getJob() << " : " << job->getPid() << endl;
        kill(job->getPid(), SIGCONT);
    }
    catch (...) {
        cerr << "smash error: bg: invalid arguments" << endl;
    }
}

void QuitCommand::execute() {
    if (cmd[1] == "kill") {
        int count = jobs->jobs->size();
        cout << "smash: sending SIGKILL signal to " << count << " jobs:" << endl;
        jobs->killAllJobs();
    }

}

RedirectionCommand::RedirectionCommand(const string cmd_line) : Command(cmd_line) {
}

string delete_2last(string *cmd) {
    string str = "";
    int count = 0;
    while (count < COMMAND_MAX_ARGS - 2 && !cmd[count].empty()) {
        if (!cmd[count + 2].empty())
            str += cmd[count];
        count++;
    }
    return str;
}

string last_word(string *cmd) {
    string str = "";
    int count = 0;
    while (!cmd[count].empty())
        count++;
    str = cmd[count];
    return str;
}

void RedirectionCommand::execute() {
    if (cmd_line.find(">>")) {

        File * f = open(last_word(cmd), O_WRONLY O_CREATE O_APPEND);
        return;
    }
    if (cmd_line.find(">")) {
        File * file = open(last_word(cmd), O_WRONLY O_CREATE);
        streambuf *stream_buffer_cout = cout.rdbuf();
        streambuf *stream_buffer_file = file.rdbuf();
        cout.rdbuf(stream_buffer_file);
        Command *comm = CreateCommand(delete_2last(cmd));

        cout.rdbuf(stream_buffer_cout);
        file.close();
        return;
    }
}

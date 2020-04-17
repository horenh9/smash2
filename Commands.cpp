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


/******************** Small Shell ************/

SmallShell::SmallShell() {
    prompt_name = "smash";
    jobs = new JobsList();
    plastPwd = "";
}

SmallShell::~SmallShell() {
    delete jobs;
}

Command *SmallShell::CreateCommand(const string cmd_line) {
    string *cmd = new string[COMMAND_MAX_ARGS];
    for (int i = 0; i < COMMAND_MAX_ARGS; ++i)
        cmd[i] = "";
    int count = _parseCommandLine(cmd_line, cmd);
    string command_name = cmd[0];
    string cmd_s = string(cmd_line);
    delete[] cmd;
    if (cmd_s.find("|") == 0)
        return new PipeCommand(cmd_line, this);
    else if (cmd_s.find(">") == 0)
        return new RedirectionCommand(cmd_line, this);
    else if ("chprompt" == command_name || (count == 1 && "chprompt&" == command_name))
        return new ChangePrompt(cmd_line, &prompt_name);
    else if ("pwd" == command_name || "pwd&" == command_name)
        return new GetCurrDirCommand(cmd_line);
    else if ("cd" == command_name)
        return new ChangeDirCommand(cmd_line, &plastPwd);
    else if ("showpid" == command_name || "showpid&" == command_name)
        return new ShowPidCommand(cmd_line);
    else if ("jobs" == command_name || "jobs&" == command_name)
        return new JobsCommand(cmd_line, jobs);
    else if ("kill" == command_name)
        return new KillCommand(cmd_line, jobs);
    else if ("fg" == command_name)
        return new ForegroundCommand(cmd_line, jobs);
    else if ("bg" == command_name)
        return new BackgroundCommand(cmd_line, jobs);
    else if ("quit" == command_name || "quit&" == command_name)
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

Command::Command(const string cmd_line, int out, int in, int err) : cmd_line(cmd_line), out(out), in(in), err(err) {
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

RedirectionCommand::RedirectionCommand(const string cmd_line, SmallShell *smash) : Command(cmd_line), smash(smash) {
}

PipeCommand::PipeCommand(const string cmd_line, SmallShell *smash) : Command(cmd_line), smash(smash) {
    int counter = 0;
    op = "";
    while (!cmd[counter].empty()) {
        if (cmd[counter] == "|" || cmd[counter] == "|&")
            if (cmd[counter] == "|")
                op = "|";
            else
                op = "|&";
        else if (op.empty())
            command1 += cmd[counter];
        else
            command2 += cmd[counter];

        counter++;
    }
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

void JobsList::printJobsList(int out) {
    removeFinishedJobs();
    for (auto it = jobs->begin(); it != jobs->end(); ++it) {
        double diff = difftime(it->getBegin(), time(0));
        string mode = "";
        if (it->getMode() == 0)
            mode = "(stopped)";
        string print = "[" + to_string(it->getJobId()) + "] " + it->getJob() + " : " + to_string(it->getPid()) + " " +
                       to_string(diff) + " secs " + mode + "\n";
        write(out, print, print.size());
    }
}

void JobsList::killAllJobs(int out) {
    for (auto it = jobs->begin(); it != jobs->end(); ++it) {
        string print = to_string(it->getPid()) + ": " + it->getJob() + "\n";
        write(out, print, print.size());
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

/******************** executes************/

void RedirectionCommand::execute() {
    int default_cout = dup(1);
    if (default_cout == -1) {
        perror("smash error: dup failed");
        return;
    }
    close(1);
    int check;
    if (cmd_line.find(">>"))
        check = open(last_word(cmd), O_WRONLY | O_CREATE | O_APPEND);
    else
        check = open(last_word(cmd), O_WRONLY | O_CREATE | O_TRUNC);
    if (check == -1) {
        dup2(default_cout, 1);
        perror("smash error: open failed");
        return;
    }
    Command *comm = smash->CreateCommand(delete_2last(cmd));
    comm->execute();
    dup2(default_cout, 1);
    close(default_cout);
}

void ExternalCommand::execute() {
    if (fork() == 0) {
        char *c = "-c";
        if (cmd_line.at(cmd_line.size()) == '&')
            jobs->addJob(this);
        const char *argv[] = {c, cmd_line.c_str(), nullptr};
        execv("/bin/bash", argv);
    } else {
        wait(nullptr);
    }
}

void PipeCommand::execute() {

}


/******************** Built in command executes************/

void ChangePrompt::execute() {
    if (cmd[1].empty() || cmd[1] == "&")
        *prompt = "smash";
    else
        *prompt = cmd[1];
}

void ShowPidCommand::execute() {
    string print = "smash pid is " + to_string(getPid()) + "\n";
    write(out, print, print.size());
}

void GetCurrDirCommand::execute() {
    char buf[COMMAND_ARGS_MAX_LENGTH];
    string print = getcwd(buf, COMMAND_ARGS_MAX_LENGTH);
    write(out, print, print.size());
}

void ChangeDirCommand::execute() {
    if (cmd[2].empty()) {
        string print = "smash error: cd: too many arguments\n";
        write(err, print, print.size());
        return;
    }
    string temp = cmd[1]; // load path to temp

    if (temp == "-") {
        if (OLDPWD->empty()) {
            string print = "smash error: cd: OLDPWD not set\n";
            write(err, print, print.size());
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
    jobs->printJobsList(out);
}

void KillCommand::execute() {
    jobs->removeFinishedJobs();
    try {
        int id = stoi(cmd[2]);
        if (jobs->getJobById(id) == nullptr) {
            string print = "smash error: kill: job-id " + cmd[2] + " does not exist\n";
            return;
        }
        if (cmd[1].at(0) != '-' || !cmd[3].empty()) {
            string print = "smash error: kill: invalid arguments\n";
            return;
        }
        string print = "signal number " + cmd[1].substr(1, cmd[1].length() - 1) + " was sent to pid "
                       + to_string(jobs->getJobById(id)->getPid()) + "\n";
        write(out, print, print.size());
        kill(jobs->getJobById(id)->getPid(), stoi(cmd[1].substr(1, cmd[1].length() - 1))));
    }
    catch (...) {
        string print = "smash error: kill: invalid arguments\n";
    }
}

void ForegroundCommand::execute() {
    jobs->removeFinishedJobs();
    if (jobs->jobs->empty() && cmd[1].empty()) {
        string print = "smash error: fg: jobs list is empty\n";
        write(err, print, print.size());
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
            string print = job->getJob() + " : " + to_string(job->getPid()) + "\n";
            write(out, print, print.size());
            kill(job->getPid(), SIGCONT);
            waitpid(job->getPid(), nullptr, 0);

        } else {
            string print = "smash error: fg: job-id " + to_string(id) + " does not exist\n";
            write(err, print, print.size());
        }
    }
    catch (...) {
        string print = "smash error: fg: invalid arguments\n";
        write(err, print, print.size());
    }
}

void BackgroundCommand::execute() {
    jobs->removeFinishedJobs();
    int id;
    JobsList::JobEntry *job = jobs->getLastStoppedJob(&id);
    if (!job && cmd[1].empty()) {
        string print = "smash error: bg: there is no stopped jobs to resume\n";
        write(err, print, print.size());
        return;
    }
    try {
        if (!cmd[1].empty()) {
            id = stoi(cmd[1]);
            job = jobs->getJobById(id);
        }
        if (!job) {
            string print = "smash error: bg: job-id " + to_string(id) + " does not exist\n";
            write(err, print, print.size());
            return;
        }
        if (job->getMode() == 1) {
            string print = " smash error: bg: job-id" + to_string(id) + "is already running in the background\n";
            write(err, print, print.size());
            return;
        }
        job->setMode(1);
        string print = job->getJob() + " : " + to_string(job->getPid()) + "\n";
        write(out, print, print.size());
        kill(job->getPid(), SIGCONT);
    }
    catch (...) {
        string print = "smash error: bg: invalid arguments\n";
        write(err, print, print.size());
    }
}

void QuitCommand::execute() {
    if (cmd[1] == "kill") {
        int count = jobs->jobs->size();
        string print = "smash: sending SIGKILL signal to " + to_string(count) + " jobs:\n";
        write(out, print, print.size());
        jobs->killAllJobs(out);
    }

}

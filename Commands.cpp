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
        args[++i] = "";
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



/******************** Small Shell ************/

SmallShell::SmallShell() : currcmd(nullptr) {
    prompt_name = "smash";
    jobs = new JobsList();
    plastPwd = "";
    pid = getpid();
}

SmallShell::~SmallShell() {
    delete jobs;
}

Command *SmallShell::CreateCommand(const string cmd_line, int out, int in, int err) {
    string *cmd = new string[COMMAND_MAX_ARGS];
    for (int i = 0; i < COMMAND_MAX_ARGS; ++i)
        cmd[i] = "";
    int count = _parseCommandLine(cmd_line, cmd);
    string command_name = cmd[0];
    string cmd_s = string(cmd_line);
    delete[] cmd;
    if (cmd_s.find("|") != string::npos)
        return new PipeCommand(cmd_line, this);
    else if (cmd_s.find(">") != string::npos)
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
    else {
        return new ExternalCommand(cmd_line, jobs, this, out, in, err);
    }
}

void SmallShell::executeCommand(const string cmd_line) {
    // TODO: Add your implementation here
    // for example:
    // Please note that you must fork smash process for some commands (e.g., external commands....)
    Command *cmd = CreateCommand(cmd_line);
    cmd->execute();
}

int SmallShell::getPid() {
    return pid;
}

void SmallShell::setPidInFG(pid_t pid) {
    curr_pid = pid;
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

/********************************/

Command::Command(const string cmd_line, int out, int in, int err) : cmd_line(cmd_line), out(out), in(in), err(err) {
    cmd = new string[COMMAND_MAX_ARGS];
    for (int i = 0; i < COMMAND_MAX_ARGS; ++i)
        cmd[i] = "";
    _parseCommandLine(cmd_line, cmd);
    // bg = _isBackgroundComamnd(cmd_line);
}

Command::~Command() {
    delete[] cmd;
}

/******************** Built in command constructors************/

ExternalCommand::ExternalCommand(const string cmd_line, JobsList *jobs, SmallShell *smash, int out, int in, int err) :
        Command(cmd_line, out, in, err), jobs(jobs), smash(smash) {}

RedirectionCommand::RedirectionCommand(const string cmd_line, SmallShell *smash) : Command(cmd_line), smash(smash) {
}

PipeCommand::PipeCommand(const string cmd_line, SmallShell *smash) :
        Command(cmd_line), smash(smash), cmd1(nullptr), cmd2(nullptr) {
    int counter = 0;
    op = "";
    while (!cmd[counter].empty()) {// split the commands
        if (cmd[counter] == "|" || cmd[counter] == "|&")
            if (cmd[counter] == "|")
                op = "|";
            else
                op = "|&";
        else if (op.empty())
            command1 += cmd[counter] + " ";
        else
            command2 += cmd[counter] + " ";
        counter++;
    }
}


PipeCommand::~PipeCommand() {
    delete cmd1;
    delete cmd2;
}

BuiltInCommand::BuiltInCommand(const string cmd_line, int out, int in, int err) : Command(cmd_line, out, in, err) {
}

ChangePrompt::ChangePrompt(const string cmd_line, string *prompt_name) :
        BuiltInCommand(cmd_line), prompt(prompt_name) {
}

JobsCommand::JobsCommand(const string cmd_line, JobsList *jobs, int out, int in, int err) :
        BuiltInCommand(cmd_line, out, in, err), jobs(jobs) {
}

ChangeDirCommand::ChangeDirCommand(const string cmd_line, string *plastPwd, int out, int in, int err) :
        BuiltInCommand(cmd_line, out, in, err) {
    OLDPWD = plastPwd;
}

KillCommand::KillCommand(const string cmd_line, JobsList *jobs, int out, int in, int err) :
        BuiltInCommand(cmd_line, out, in, err), jobs(jobs) {
}

ForegroundCommand::ForegroundCommand(const string cmd_line, JobsList *jobs, SmallShell *smash, int out, int in, int err)
        : BuiltInCommand(cmd_line, out, in, err), jobs(jobs), smash(smash) {
}

BackgroundCommand::BackgroundCommand(const string cmd_line, JobsList *jobs, int out, int in, int err) :
        BuiltInCommand(cmd_line, out, in, err), jobs(jobs) {
}

GetCurrDirCommand::GetCurrDirCommand(const string cmd_line, int out, int in, int err) :
        BuiltInCommand(cmd_line, out, in, err) {
}

ShowPidCommand::ShowPidCommand(const string cmd_line, SmallShell *smash, int out, int in, int err) :
        BuiltInCommand(cmd_line, out, in, err), smash(smash) {
}

QuitCommand::QuitCommand(const string cmd_line, JobsList *jobs, int out, int in, int err) :
        BuiltInCommand(cmd_line, out, in, err), jobs(jobs) {
}

/******************** Jobs ************/

JobsList::JobsList() : max(0) {
    jobs_list = new list<JobEntry>();
}

JobsList::~JobsList() {
    delete jobs_list;
}

JobsList::JobEntry::JobEntry(string *job, int jobId, int pid, int mode, Command *cmd) :
        jobId(jobId), pid(pid), mode(mode), begin(time(0)), cmd(cmd) {
    int i = 0;
    job_name = "";
    while (job[i] != "") {
        job_name += job[i] + " ";
        i++;
    }
    job_name.substr(job_name.size(), 1);
}

int JobsList::addJob(Command *cmd, int pid, int mode) {
    removeFinishedJobs();
    JobEntry job = JobEntry(cmd->getJob(), max + 1, pid, mode, cmd);
    jobs_list->push_back(job);
    return job.getJobId();
}

void JobsList::printJobsList(int out) {
    removeFinishedJobs();
    for (auto it = jobs_list->begin(); it != jobs_list->end(); ++it) {
        double diff = difftime(time(0), it->getBegin());
        string mode = "";
        if (it->getMode() == 0)
            mode = "(stopped)";
        string print = "[" + to_string(it->getJobId()) + "] " + it->getJob() + " : " + to_string(it->getPid()) + " " +
                       to_string(int(diff)) + " secs " + mode + "\n";
        const char *p = print.c_str();
        write(out, p, print.size());
    }
}

void JobsList::killAllJobs(int out) {
    for (auto it = jobs_list->begin(); it != jobs_list->end(); ++it) {
        string print = to_string(it->getPid()) + ": " + it->getJob() + "\n";
        const char *p = print.c_str();
        write(out, p, print.size());
        KillCommand killcmd = KillCommand("kill -9 " + to_string(it->getJobId()), this);
        killcmd.execute();
        ++it;
        removeJobById((--it)->getJobId());
        --it;
    }
}

void JobsList::removeFinishedJobs() {
    int currMax = 0, check = 0;
    for (auto it = jobs_list->begin(); it != jobs_list->end(); ++it) {
        if (waitpid(it->getPid(), &check, WNOHANG) != 0) {
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
    for (auto it = jobs_list->begin(); it != jobs_list->end(); ++it)
        if (it->getJobId() == jobId)
            return &(*it);
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
    for (auto it = jobs_list->begin(); it != jobs_list->end(); ++it)
        if (it->getJobId() == max)
            return &(*it);
    return nullptr;
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId) {
    JobEntry *maxStopped = &(*(jobs_list->begin()));
    (*jobId) = 0;
    for (auto it = jobs_list->begin(); it != jobs_list->end(); ++it)
        if (it->getJobId() > maxStopped->getJobId() && it->getMode() == 0)
            maxStopped = &(*it);
    if (maxStopped != nullptr)
        (*jobId) = maxStopped->getJobId();
    return maxStopped;
}

void JobsList::addJob(JobEntry *job) {
    jobs_list->push_back(*job);
}

/******************** Getters/Setters************/

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

/******************** Built in command executes************/

void ChangePrompt::execute() {
    if (cmd[1].empty() || cmd[1] == "&")
        *prompt = "smash";
    else
        *prompt = cmd[1];
}

void ShowPidCommand::execute() {
    string print = "smash pid is " + to_string(smash->getPid()) + "\n";
    const char *p = print.c_str();
    write(out, p, print.size());
}

void GetCurrDirCommand::execute() {
    char buf[COMMAND_ARGS_MAX_LENGTH];
    string print = getcwd(buf, COMMAND_ARGS_MAX_LENGTH);
    if (print.c_str() == nullptr) {
        perror("smash error: getcwd faild");
        return;
    }
    print += "\n";
    const char *p = print.c_str();
    write(out, p, print.size());
}

void ChangeDirCommand::execute() {
    if (!cmd[2].empty()) {
        string print = "smash error: cd: too many arguments\n";
        const char *p = print.c_str();
        write(err, p, print.size());
        return;
    }
    string temp = cmd[1]; // load path to temp

    if (temp == "-") {
        if (OLDPWD->empty()) {
            string print = "smash error: cd: OLDPWD not set\n";
            const char *p = print.c_str();
            write(err, p, print.size());
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
        if (chdir(temp.data()) == -1)        //go to wanted directory via syscall
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
            const char *p = print.c_str();
            write(out, p, print.size());
            return;
        }
        if (cmd[1].at(0) != '-' || !cmd[3].empty()) {
            string print = "smash error: kill: invalid arguments\n";
            const char *p = print.c_str();
            write(out, p, print.size());
            return;
        }
        string print = "signal number " + cmd[1].substr(1, cmd[1].length() - 1) + " was sent to pid "
                       + to_string(jobs->getJobById(id)->getPid()) + "\n";
        const char *p = print.c_str();
        write(out, p, print.size());
        kill(jobs->getJobById(id)->getPid(), stoi(cmd[1].substr(1, cmd[1].length() - 1)));
    }
    catch (...) {
        string print = "smash error: kill: invalid arguments\n";
        const char *p = print.c_str();
        write(out, p, print.size());
    }
}

void ForegroundCommand::execute() {
    jobs->removeFinishedJobs();
    int check;
    if (jobs->jobs_list->empty() && cmd[1].empty()) {
        string print = "smash error: fg: jobs list is empty\n";
        const char *p = print.c_str();
        write(err, p, print.size());
        return;
    }
    try {
        int id;
        jobs->getLastJob(&id);
        if (!cmd[1].empty()) {
            id = stoi(cmd[1]);
        }
        JobsList::JobEntry *job = jobs->getJobById(id);
        if (!cmd[2].empty()) {
            string print = "smash error: fg: invalid arguments\n";
            const char *p = print.c_str();
            write(err, p, print.size());
            return;
        }
        if (job) {
            job->setMode(2);
            jobs->removeJobById(id);
            string print = job->getJob() + " : " + to_string(job->getPid()) + "\n";
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
                kill(job->getPid(), SIGCONT);
                exit(2);

            } else {
                waitpid(job->getPid(), &check, WUNTRACED);
                smash->setCommand(nullptr);
                smash->setJob(nullptr);
                smash->setPidInFG(-1);
            }

        } else {
            string print = "smash error: fg: job-id " + to_string(id) + " does not exist\n";
            const char *p = print.c_str();
            write(err, p, print.size());
        }
    }
    catch (...) {
        string print = "smash error: fg: invalid arguments\n";
        const char *p = print.c_str();
        write(err, p, print.size());
    }
}

void BackgroundCommand::execute() {
    jobs->removeFinishedJobs();
    int id;
    JobsList::JobEntry *job = jobs->getLastStoppedJob(&id);
    if (!job && cmd[1].empty()) {
        string print = "smash error: bg: there is no stopped jobs to resume\n";
        const char *p = print.c_str();
        write(err, p, print.size());
        return;
    }
    try {
        if (!cmd[1].empty()) {
            id = stoi(cmd[1]);
            job = jobs->getJobById(id);
        }
        if (!job) {
            string print = "smash error: bg: job-id " + to_string(id) + " does not exist\n";
            const char *p = print.c_str();
            write(err, p, print.size());
            return;
        }
        if (job->getMode() == 1) {
            string print = " smash error: bg: job-id " + to_string(id) + " is already running in the background\n";
            const char *p = print.c_str();
            write(err, p, print.size());
            return;
        }
        if (!cmd[2].empty()) { // there is more than one argument
            string print = "smash error: fg: invalid arguments\n";
            const char *p = print.c_str();
            write(err, p, print.size());
            return;
        }
        job->setMode(1);
        string print = job->getJob() + " : " + to_string(job->getPid()) + "\n";
        const char *p = print.c_str();
        write(out, p, print.size());
        kill(job->getPid(), SIGCONT);
    }
    catch (...) {
        string print = "smash error: bg: invalid arguments\n";
        const char *p = print.c_str();
        write(err, p, print.size());
    }
}

void QuitCommand::execute() {
    if (cmd[1] == "kill" || cmd[1] == "kill&") {
        int count = jobs->jobs_list->size();
        string print = "smash: sending SIGKILL signal to " + to_string(count) + " jobs:\n";
        const char *p = print.c_str();
        write(out, p, print.size());
        jobs->killAllJobs(out);
    }
    exit(2);
}

/******************** executes************/
void arrange_cmd(string cmd_line, string *actual_cmd, string *file) {
    int pos1 = cmd_line.find_first_of('>');
    int pos2 = cmd_line.find_last_of('>');
    if (pos1 == pos2) {// '>' redirection
        *actual_cmd = _trim(cmd_line.substr(0, pos1));
        *file = _trim(cmd_line.substr(pos1 + 1, cmd_line.length() - pos1 - 1));
    } else {// '>>' redirection
        *actual_cmd = _trim(cmd_line.substr(0, pos1));
        *file = _trim(cmd_line.substr(pos2 + 1, cmd_line.length() - pos2 - 1));
    }
}

void RedirectionCommand::execute() {
    int fd;
    string actual_cmd;
    string file_name;
    arrange_cmd(cmd_line, &actual_cmd, &file_name);
    if (cmd_line.find(">>") != string::npos)
        fd = open(file_name.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);
    else
        fd = open(file_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("smash error: open failed");
        return;
    }
    Command *comm = smash->CreateCommand(actual_cmd, fd);
    comm->execute();
    if (close(fd) == -1)
        perror("smash error: close failed");
}

void ExternalCommand::execute() {
    int check;
    char *c = (char *) "-c";
    bool bg = _isBackgroundComamnd(cmd_line);
    smash->setCommand(this);
    pid_t extpid = fork();
    smash->setPidInFG(extpid);
    string temp;
    if (bg) {
        jobs->addJob(this, extpid, 1);
        temp = cmd_line.substr(0, cmd_line.size() - 1);
    } else
        temp = cmd_line;
    if (extpid == 0) {//son
        setpgrp();
        dup2(in, 0);
        dup2(out, 1);
        dup2(err, 2);
        char *argv[] = {(char *) "/bin/bash", c, (char *) temp.c_str(), nullptr};
        execv("/bin/bash", argv);

    } else if (!bg) { //father
        waitpid(extpid, &check, WUNTRACED);
        smash->setJob(nullptr);
        smash->setPidInFG(-1);
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

    pid_t pid1 = fork();
    if (pid1 == 0) { //son
        close(fd[0]);
        cmd1->execute();
        close(fd[1]);
    } else { //father
        close(fd[1]);
        pid_t pid2 = fork();
        if (pid2 == 0) {
            cmd2->execute();
            close(fd[0]);
        } else {
            close(fd[0]);
            waitpid(pid1, nullptr, WNOHANG);
            waitpid(pid2, nullptr, WNOHANG);
        }

    }
}

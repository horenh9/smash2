#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string.h>
#include <list>
#include <time.h>

using namespace std;
#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class JobsList;

class SmallShell;

class Command {
protected:
    const string cmd_line;
    string *cmd;
    int out;
    int in;
    int err;
public:

    string *getJob() const;

    explicit Command(const string& cmd_line, int out = 1, int in = 0, int err = 2);

    virtual ~Command();

    virtual void execute() = 0;

    //virtual void prepare();
    //virtual void cleanup();
};

class JobsList {
public:
    class JobEntry {
        string job_name;
        int jobId;
        int pid;
        int mode;//0 = stopped, 1 = bg, 2 = fg, 3 = done
        time_t begin;
        Command *cmd;

    public:
        int getJobId() const;

        string getJob() const;

        int getPid() const;

        time_t getBegin() const;

        int getMode() const;

        void setMode(int mode);

        Command *getCommand();

        JobEntry(string *job, int jobId, int pid, int mode, Command *cmd);
    };

    list <JobEntry> *jobs_list;
    int max;

    JobsList();

    ~JobsList();

    int addJob(Command *cmd, int pid, int mode);

    void addJob(JobEntry *job);

    void printJobsList(int out);

    void killAllJobs(int out);

    void removeJobById(int jobId);

    void removeFinishedJobs();

    JobEntry *getJobById(int jobId);

    JobEntry *getLastJob(int *lastJobId);

    JobEntry *getLastStoppedJob(int *jobId);
};

class ExternalCommand : public Command {
    JobsList *jobs;
    SmallShell *smash;
public:
    explicit ExternalCommand(const string& cmd_line, JobsList *jobs, SmallShell *smash,
                             int out = 1, int in = 0, int err = 2);

    ~ExternalCommand() override = default;

    void execute() override;
};

class PipeCommand : public Command {
    SmallShell *smash;
    string op;
    string command1;
    string command2;
    Command *cmd1;
    Command *cmd2;
public:
    explicit PipeCommand(const string& cmd_line, SmallShell *smash);

    ~PipeCommand() override;

    void execute() override;
};

class RedirectionCommand : public Command {
    SmallShell *smash;
public:
    explicit RedirectionCommand(const string& cmd_line, SmallShell *smash);

    ~RedirectionCommand() override = default;

    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};

class BuiltInCommand : public Command {
public:
    explicit BuiltInCommand(const string& cmd_line, int out = 1, int in = 0, int err = 2);

    ~BuiltInCommand() override = default;
};

class ChangePrompt : public BuiltInCommand {
    string *prompt;
public:
    explicit ChangePrompt(const string& cmd_line, string *prompt_name);

    ~ChangePrompt() override = default;

    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
    SmallShell *smash;
public:
    explicit ShowPidCommand(const string& cmd_line, SmallShell *smash, int out = 1, int in = 0, int err = 2);

    ~ShowPidCommand() override = default;;

    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    explicit GetCurrDirCommand(const string& cmd_line, int out = 1, int in = 0, int err = 2);

    ~GetCurrDirCommand() override = default;;

    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
    string *OLDPWD;
public:
    ChangeDirCommand(const string& cmd_line, string *plastPwd, int out = 1, int in = 0, int err = 2);

    ~ChangeDirCommand() override = default;;

    void execute() override;
};

class JobsCommand : public BuiltInCommand {
    JobsList *jobs;
public:
    JobsCommand(const string& cmd_line, JobsList *jobs, int out = 1, int in = 0, int err = 2);

    ~JobsCommand() override = default;;

    void execute() override;
};

class KillCommand : public BuiltInCommand {
    JobsList *jobs;
public:
    KillCommand(const string& cmd_line, JobsList *jobs, int out = 1, int in = 0, int err = 2);

    ~KillCommand() override = default;;

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    JobsList *jobs;
    SmallShell *smash;
public:
    ForegroundCommand(const string& cmd_line, JobsList *jobs, SmallShell *smash, int out = 1, int in = 0, int err = 2);

    ~ForegroundCommand() override = default;

    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList *jobs;
public:
    BackgroundCommand(const string& cmd_line, JobsList *jobs, int out = 1, int in = 0, int err = 2);

    ~BackgroundCommand() override = default;

    void execute() override;
};

class QuitCommand : public BuiltInCommand {
    JobsList *jobs;
public:
    QuitCommand(const string& cmd_line, JobsList *jobs, int out = 1, int in = 0, int err = 2);

    ~QuitCommand() override = default;;

    void execute() override;
};

// TODO: should it really inhirit from BuiltInCommand ?
class CopyCommand : public BuiltInCommand {
public:
    CopyCommand(const string cmd_line);

    virtual ~CopyCommand() {};

    void execute() override;
};

// TODO: add more classes if needed 
// maybe chprompt , timeout ?

class SmallShell {
    int pid;
    string plastPwd;
    pid_t curr_pid;
    Command *currcmd;
    JobsList::JobEntry *currjob;

public:
    JobsList *jobs;

    string prompt_name;

    SmallShell();

    int getPid();

    int getPidInFG();

    void setJob(JobsList::JobEntry *job);

    JobsList::JobEntry *getJob();

    void setCommand(Command *cmd);

    Command *getCommand();

    void setPidInFG(pid_t pid);

    Command *CreateCommand(const string& cmd_line, int out = 1, int in = 0, int err = 2);

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

    ~SmallShell();

    void executeCommand(const string cmd_line);
};

#endif //SMASH_COMMAND_H_

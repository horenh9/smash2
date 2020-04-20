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

    Command(const string cmd_line, int out = 1, int in = 0, int err = 2);

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
    public:
        int getJobId() const;

        string getJob() const;

        int getPid() const;

        time_t getBegin() const;

        int getMode() const;

        void setMode(int mode);
        void setPid(int pid);

        JobEntry(string *job, int jobId, int pid, int mode);
    };

    list <JobEntry> *jobs_list;
    int max;

    JobsList();

    ~JobsList();

    int addJob(Command *cmd, int pid, int mode);

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
public:
    explicit ExternalCommand(const string cmd_line, JobsList *jobs, int out = 1, int in = 0, int err = 2);

    virtual ~ExternalCommand() {};

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
    explicit PipeCommand(const string cmd_line, SmallShell *smash);

    virtual ~PipeCommand();

    void execute() override;
};

class RedirectionCommand : public Command {
    SmallShell *smash;
public:
    explicit RedirectionCommand(const string cmd_line, SmallShell *smash);

    virtual ~RedirectionCommand() = default;

    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};


class BuiltInCommand : public Command {
public:
    explicit BuiltInCommand(const string cmd_line, int out = 1, int in = 0, int err = 2);

    virtual ~BuiltInCommand() {};
};

class ChangePrompt : public BuiltInCommand {
    string *prompt;
public:
    explicit ChangePrompt(const string cmd_line, string *prompt_name);

    virtual ~ChangePrompt() {};

    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
    SmallShell *smash;
public:
    explicit ShowPidCommand(const string cmd_line, SmallShell *smash, int out = 1, int in = 0, int err = 2);

    virtual ~ShowPidCommand() {};

    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    explicit GetCurrDirCommand(const string cmd_line, int out = 1, int in = 0, int err = 2);

    virtual ~GetCurrDirCommand() {};

    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
    string *OLDPWD;
public:
    ChangeDirCommand(const string cmd_line, string *plastPwd, int out = 1, int in = 0, int err = 2);

    virtual ~ChangeDirCommand() {};

    void execute() override;
};

class JobsCommand : public BuiltInCommand {
    JobsList *jobs;
public:
    JobsCommand(const string cmd_line, JobsList *jobs, int out = 1, int in = 0, int err = 2);

    virtual ~JobsCommand() {};

    void execute() override;
};

class KillCommand : public BuiltInCommand {
    JobsList *jobs;
public:
    KillCommand(string cmd_line, JobsList *jobs, int out = 1, int in = 0, int err = 2);

    virtual ~KillCommand() {};

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    JobsList *jobs;
public:
    ForegroundCommand(const string cmd_line, JobsList *jobs, int out = 1, int in = 0, int err = 2);

    virtual ~ForegroundCommand() {};

    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList *jobs;
public:
    BackgroundCommand(const string cmd_line, JobsList *jobs, int out = 1, int in = 0, int err = 2);

    virtual ~BackgroundCommand() {};

    void execute() override;
};

class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:
    JobsList *jobs;
public:
    QuitCommand(const string cmd_line, JobsList *jobs, int out = 1, int in = 0, int err = 2);

    virtual ~QuitCommand() {};

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
    JobsList *jobs;
    int pid;
    string plastPwd;

public:

    string prompt_name;

    SmallShell();

    int getPid();

    Command *CreateCommand(const string cmd_line, int out = 1, int in = 0, int err = 2);

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
    // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_

#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string.h>
#include <list>
#include <time.h>

using namespace std;
#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)

class Command {
    // TODO: Add your data members
protected:
    char **cmd;
    bool bg;
    int size;
    int pid;
public:
    string getJob() const; //comment
    int getPid() const;

    Command(const char *cmd_line);

    virtual ~Command() = default;

    virtual void execute() = 0;

    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
    bool getBg() const;
};

class BuiltInCommand : public Command {
public:
    //explicit BuiltInCommand(const char *cmd_line);
    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
public:
    ExternalCommand(const char *cmd_line);

    virtual ~ExternalCommand() {}

    void execute() override;
};

class PipeCommand : public Command {
    // TODO: Add your data members
public:
    PipeCommand(const char *cmd_line);

    virtual ~PipeCommand() {}

    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
public:
    explicit RedirectionCommand(const char *cmd_line);

    virtual ~RedirectionCommand() {}

    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members
    char **OLDPWD;
public:
    ChangeDirCommand(const char *cmd_line, char **plastPwd);

    virtual ~ChangeDirCommand() {}

    void execute() override;

};

class GetCurrDirCommand : public BuiltInCommand {
public:
 //   GetCurrDirCommand(const char *cmd_line);

    virtual ~GetCurrDirCommand() {}

    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    //   ShowPidCommand(const char *cmd_line);

    virtual ~ShowPidCommand() {}

    void execute() override;
};

class JobsList;

class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:
    QuitCommand(const char *cmd_line, JobsList *jobs);

    virtual ~QuitCommand() {}

    void execute() override;
};

class CommandsHistory {
protected:
    class CommandHistoryEntry {
        // TODO: Add your data members
    };
    // TODO: Add your data members
public:
    CommandsHistory();

    ~CommandsHistory() {}

    void addRecord(const char *cmd_line);

    void printHistory();
};

class HistoryCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    HistoryCommand(const char *cmd_line, CommandsHistory *history);

    virtual ~HistoryCommand() {}

    void execute() override;
};

class JobsList {
public:
    class JobEntry {
        const string job;
        int jobId;
        const int pid;
        int mode;//0 = stopped, 1 = bg, 2 = fg, 3 = done
        const time_t begin;
    public:
        int getJobId() const;

        const string getJob() const;

        int getPid() const;

        time_t getBegin() const;

        void setJobId(int jobId);

        int getMode() const;

        void setMode(int mode);

        JobEntry(string job, int jobId, int pid, int mode);
    };

    // TODO: Add your data members
    list <JobEntry> *jobs;
    int max;

    JobsList();

    ~JobsList();

    void addJob(Command *cmd, bool isStopped = false);

    void printJobsList();

    void killAllJobs();//comment

    void removeJobById(int jobId);

    void removeFinishedJobs();

    JobEntry *getJobById(int jobId);//comment

    JobEntry *getLastJob(int *lastJobId);

    JobEntry *getLastStoppedJob(int *jobId);
    // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    JobsCommand(const char *cmd_line, JobsList *jobs);

    virtual ~JobsCommand() {}

    void execute() override;
};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    KillCommand(const char *cmd_line, JobsList *jobs);

    virtual ~KillCommand() {}

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    ForegroundCommand(const char *cmd_line, JobsList *jobs);

    virtual ~ForegroundCommand() {}

    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    BackgroundCommand(const char *cmd_line, JobsList *jobs);

    virtual ~BackgroundCommand() {}

    void execute() override;
};


// TODO: should it really inhirit from BuiltInCommand ?
class CopyCommand : public BuiltInCommand {
public:
    CopyCommand(const char *cmd_line);

    virtual ~CopyCommand() {}

    void execute() override;
};

// TODO: add more classes if needed 
// maybe chprompt , timeout ?

class SmallShell {
private:
    // TODO: Add your data members
    SmallShell();

public:
    Command *CreateCommand(const char *cmd_line);

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

    ~SmallShell();

    void executeCommand(const char *cmd_line);
    // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_

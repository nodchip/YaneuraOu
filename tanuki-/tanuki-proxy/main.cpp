#include <Windows.h>

#include "../../src/common.hpp"

namespace
{

  class Process
  {
  public:
    Process(const std::string& command);
    virtual ~Process();
    bool Open();
    bool Write(const std::string& line) const;
    bool WriteAsync(const std::string& line) const;
    bool StartReadAsync() const;

  private:
    const std::string command;
    // https://support.microsoft.com/ja-jp/kb/190351
    HANDLE outputRead;
    HANDLE inputWrite;
    PROCESS_INFORMATION processInformation;
  };

  std::vector<std::unique_ptr<Process> > processes;

  bool WriteToAllProcesses(const std::string& line)
  {
    for (const auto& process : processes) {
      if (!process->Write(line)) {
        return false;
      }
    }
    return true;
  }

  bool WriteAsyncToAllProcesses(const std::string& line)
  {
    for (const auto& process : processes) {
      if (!process->WriteAsync(line)) {
        return false;
      }
    }
    return true;
  }

  bool OnReadAsync(const std::string& line)
  {
    static std::mutex mutex;
    std::lock_guard<std::mutex> lockGuard(mutex);
    SYNCCOUT << line << SYNCENDL;
  }

  Process::Process(const std::string& command)
    : command(command),
    outputRead(nullptr),
    inputWrite(nullptr)
  {
  }

  Process::~Process()
  {
    Write("quit");
  }

  bool Process::Open()
  {
    SECURITY_ATTRIBUTES securityAttributes = { 0 };
    securityAttributes.nLength = sizeof(securityAttributes);
    securityAttributes.bInheritHandle = TRUE;
    HANDLE outputWrite = nullptr;
    HANDLE inputRead = nullptr;
    if (!CreatePipe(&outputRead, &outputWrite, &securityAttributes, 0)) {
      return false;
    }

    if (!CreatePipe(&inputRead, &inputWrite, &securityAttributes, 0)) {
      return false;
    }

    STARTUPINFO startupinfo = { 0 };
    startupinfo.cb = sizeof(startupinfo);
    startupinfo.dwFlags = STARTF_USESTDHANDLES;
    startupinfo.hStdOutput = outputWrite;
    startupinfo.hStdInput = inputRead;

    char mutableCommand[1024];
    strcpy(mutableCommand, command.c_str());
    if (!CreateProcess(nullptr, mutableCommand, nullptr, nullptr, TRUE,
      CREATE_NEW_CONSOLE, nullptr, nullptr, &startupinfo,
      &processInformation)) {
      return false;
    }

    return true;
  }

  bool Process::Write(const std::string& line) const
  {

  }

  bool Process::WriteAsync(const std::string& line) const
  {
  }

  bool Process::StartReadAsync() const
  {
  }
}

int main()
{

  return 0;
}

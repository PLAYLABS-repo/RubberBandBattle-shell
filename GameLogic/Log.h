#pragma once

#include <fstream>
#include <string>

enum LogLevel
{
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_DEBUG
};

class FileLogger
{
public:
    FileLogger();
    FileLogger(const std::string& filename);
    ~FileLogger();

    bool Open(const std::string& filename);
    void Close();

    void Log(LogLevel level, const std::string& message);

    void Info(const std::string& message);
    void Warning(const std::string& message);
    void Error(const std::string& message);
    void Debug(const std::string& message);

private:
    std::ofstream m_file;

    const char* GetLevelString(LogLevel level) const;
};

#include "Log.h"

FileLogger::FileLogger()
{
}

FileLogger::FileLogger(const std::string& filename)
{
    Open(filename);
}

FileLogger::~FileLogger()
{
    Close();
}

bool FileLogger::Open(const std::string& filename)
{
    Close();

    m_file.open(filename.c_str(), std::ios::out);

    return m_file.is_open();
}

void FileLogger::Close()
{
    if (m_file.is_open())
        m_file.close();
}

void FileLogger::Log(LogLevel level, const std::string& message)
{
    if (!m_file.is_open())
        return;

    m_file
        << "["
        << GetLevelString(level)
        << "] "
        << message
        << std::endl;

    m_file.flush();
}

void FileLogger::Info(const std::string& message)
{
    Log(LOG_INFO, message);
}

void FileLogger::Warning(const std::string& message)
{
    Log(LOG_WARNING, message);
}

void FileLogger::Error(const std::string& message)
{
    Log(LOG_ERROR, message);
}

void FileLogger::Debug(const std::string& message)
{
    Log(LOG_DEBUG, message);
}

const char* FileLogger::GetLevelString(LogLevel level) const
{
    switch(level)
    {
        case LOG_INFO:    return "INFO";
        case LOG_WARNING: return "WARNING";
        case LOG_ERROR:   return "ERROR";
        case LOG_DEBUG:   return "DEBUG";
    }

    return "UNKNOWN";
}

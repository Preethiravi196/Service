#include "logger.h"
#include "Utilities.h"

string CLogger::m_sFileName;
CLogger* CLogger::m_pThis = NULL;
ofstream CLogger::m_Logfile;

CLogger::CLogger(string filename)
{
    m_sFileName = filename;
}

CLogger* CLogger::GetLogger(string filename)
{
    if (m_pThis == NULL) {
        m_pThis = new CLogger(filename);
        m_Logfile.open(m_sFileName.c_str(), ios::out | ios::app);
    }
    return m_pThis;
}

void CLogger::Log(const string& sMessage)
{
    m_Logfile << Util::CurrentDateTime() << ":\t";
    m_Logfile << sMessage << "\n";
}

void CLogger::Log(const char* format, ...)
{
    char* sMessage = NULL;
    int nLength = 0;
    va_list args;
    va_start(args, format);
    //  Return the number of characters in the string referenced the list of arguments.
    // _vscprintf doesn't count terminating '\0' (that's why +1)
    nLength = _vscprintf(format, args) + 1;
    sMessage = new char[nLength];
    vsprintf_s(sMessage, nLength, format, args);
    //vsprintf(sMessage, format, args);
    m_Logfile << Util::CurrentDateTime() << ":\t";
    m_Logfile << sMessage << "\n";
    va_end(args);

    delete[] sMessage;
}

CLogger& CLogger::operator<<(const string& sMessage)
{
    m_Logfile << "\n" << Util::CurrentDateTime() << ":\t";
    m_Logfile << sMessage << "\n";
    return *this;
}

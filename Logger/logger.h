#include <fstream>
#include <iostream>
#include <string>

using namespace std;

// Singleton Logger Class.
class CLogger
{
private:

    //Default constructor for the Logger class.
    CLogger(string);

    //Log file name.
    static std::string m_sFileName;

    //Singleton logger class object pointer.
    static CLogger* m_pThis;

    //Log file stream object.
    static ofstream m_Logfile;

public:

    // Logs a message
    void Log(const std::string& sMessage);

    //Variable Length Logger function
    void Log(const char* format, ...);

    //<< overloaded function to Logs a message
    CLogger& operator<<(const string& sMessage);

    // Funtion to create the instance of logger class.
    static CLogger* GetLogger(string);

    //copy constructor for the Logger class.
    CLogger(const CLogger&) = delete;

    //assignment operator for the Logger class.
    CLogger& operator=(const CLogger&) = delete;
};


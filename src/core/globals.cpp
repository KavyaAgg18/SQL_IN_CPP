#include "globals.h"
#include <ctime>
#include <string>
using namespace std;

unordered_map<string, int> datatype = {
    {"INT", 0},
    {"FLOAT", 1},
    // {"NUMBER", 1},
    {"BOOL",2},
    {"BOOLEAN", 2},
    {"STRING", 3},
    {"DATE", 4}
};
unordered_map<int,string> datatypeName = {
    {0,"INT"},
    {1,"FLOAT"},
    {2,"BOOL"},
    {3,"STRING"},
    {4,"DATE"}
};


string currentDateTime()
{
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
    return string(buf);
}

#ifdef __EMSCRIPTEN__
const string RESET = "";
const string RED = "";
const string GREEN = "";
const string ORANGE = "";
#else
const string RESET = "\033[0m";
const string RED = "\033[31m";
const string GREEN = "\033[32m";
const string ORANGE = "\033[33m";
#endif
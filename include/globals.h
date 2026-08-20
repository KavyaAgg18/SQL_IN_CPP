#ifndef GLOBALS_H
#define GLOBALS_H

#include <unordered_map>
#include <string>

using namespace std;

extern unordered_map<string, int> datatype;
extern unordered_map<int, string> datatypeName;
string currentDateTime();

extern const string RESET;
extern const string RED;
extern const string GREEN;
extern const string ORANGE;

#endif // GLOBALS_H
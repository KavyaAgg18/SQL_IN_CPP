#ifndef DATABASE_H
#define DATABASE_H

#include <iostream>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <string>
#include <ctime> // For currentDateTime()

using namespace std;
namespace fs = filesystem;

class Database
{
private:
    string name; // Database name
    vector<string> tables; // List of tables
public:
    Database();
    Database(string dbName);
    bool isValid() const;
    bool tableExists(const string &tableName);
    string getName() const;
    void displayTables() const;
    
    void addTable(const string &tableName);
    void drop(const string &tableName);
    bool renameTable(const string &oldName, const string &newName);
};

Database selectDatabase(const string &dbName);
Database createDatabase(const string &dbName);
void displayDatabases();
void initializeDatabaseSystem();
#endif // DATABASE_H
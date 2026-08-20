#include "database.h"
#include "globals.h"
#include <ctime>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
using namespace std;

void initializeDatabaseSystem()
{
    std::filesystem::path dbFolder = "Databases";
    std::filesystem::path schemaFile = dbFolder / "information_schema.csv";
    std::filesystem::path baseDb = dbFolder / "baseDb";

    if (!std::filesystem::exists(dbFolder))
    {
        std::filesystem::create_directory(dbFolder);
    }
    if (!std::filesystem::exists(schemaFile))
    {
        ofstream schema(schemaFile);
        if (schema.is_open())
        {
            schema << "database_name,date_created\n";
            schema << "baseDb," << currentDateTime() << endl;
            schema.close();
            cout << GREEN << "Created information_schema.csv file." << RESET << endl;
        }
        else
        {
            cerr << RED << "Failed to create information_schema.csv file." << RESET << endl;
        }
    }
    if (!std::filesystem::exists(baseDb))
    {
        std::filesystem::create_directory(baseDb);
    }
    if (!std::filesystem::exists(baseDb / "tables.csv"))
    {
        ofstream tables(baseDb / "tables.csv");
        if (tables.is_open())
        {
            tables << "table_name,date_created\n";
            tables.close();
            cout << GREEN << "Created tables.csv file in baseDb." << RESET << endl;
        }
        else
        {
            cerr << RED << "Failed to create tables.csv file." << RESET << endl;
        }
    }
}

Database::Database()
{
    name = "";
}

Database::Database(string dbName)
{
    if (dbName.empty() || dbName == "")
    {
        cerr << RED << "Database name cannot be empty" << RESET << endl;
        return;
    }
    name = dbName;
}

string Database::getName() const
{
    return name;
}

Database createDatabase(const string &dbName)
{
    ifstream file("./Databases/information_schema.csv");
    if (!file.is_open())
    {
        cerr << RED << "Failed to open information_schema.csv" << RESET << endl;
        throw runtime_error("Failed to open information_schema.csv");
    }

    string line;
    while (getline(file, line))
    {
        stringstream ss(line);
        string database_name, date_created;
        getline(ss, database_name, ',');

        if (database_name == dbName)
        {
            file.close();
            cerr << ORANGE << "Database already exists" << RESET << endl;
            throw runtime_error("Database already exists");
        }
    }
    file.close();

    Database db(dbName);

    // Create new database directory
    filesystem::create_directories("./Databases/" + dbName);

    ofstream infoFile("./Databases/information_schema.csv", ios::app);
    infoFile << dbName << "," << currentDateTime() << endl;
    infoFile.close();

    ofstream tableFile("./Databases/" + dbName + "/tables.csv");
    tableFile << "table_name,date_created" << endl;
    tableFile.close();

    cout << GREEN << "Database " << dbName << " created successfully." << RESET << endl;

    return db;
}

Database selectDatabase(const string &dbName)
{
    ifstream file("./Databases/information_schema.csv");
    if (!file.is_open())
    {
        cerr << RED << "Failed to open information_schema.csv" << RESET << endl;
        return Database();
    }

    string line;
    bool databaseExists = false;

    while (getline(file, line))
    {
        stringstream ss(line);
        string database_name, date_created;
        getline(ss, database_name, ',');

        if (database_name == dbName)
        {
            databaseExists = true;
            break;
        }
    }
    file.close();

    if (!databaseExists)
    {
        cerr << RED << "Database does not exist" << RESET << endl;
        return Database();
    }

    Database db(dbName);

    ifstream dbFile("./Databases/" + dbName + "/tables.csv");
    if (!dbFile.is_open())
    {
        cerr << RED << "Failed to open " << dbName << "/tables.csv" << RESET << endl;
        return Database();
    }

    while (getline(dbFile, line))
    {
        stringstream ss(line);
        string table_name;
        getline(ss, table_name, ',');
        if (!table_name.empty() && table_name != "table_name")
        {
            db.addTable(table_name);
        }
    }
    dbFile.close();

    cout << GREEN << "Database " << dbName << " selected successfully." << RESET << endl;

    return db;
}

bool Database::tableExists(const string &tableName)
{
    return find(tables.begin(), tables.end(), tableName) != tables.end();
}

void Database::addTable(const string &tableName)
{
    tables.push_back(tableName);
}

bool Database::isValid() const
{
    return !name.empty();
}

void displayDatabases()
{
    ifstream file("./Databases/information_schema.csv");
    if (!file.is_open())
    {
        cerr << RED << "Failed to open information_schema.csv" << RESET << endl;
        throw runtime_error("Failed to open information_schema.csv");
    }

    string line;
    bool isFirstLine = true;
    cout << "+----------------------+---------------------+" << endl;
    cout << "| Database Name        | Date Created        |" << endl;
    cout << "+----------------------+---------------------+" << endl;
    while (getline(file, line))
    {
        if (isFirstLine)
        {
            isFirstLine = false;
            continue;
        }
        stringstream ss(line);
        string database_name, date_created;
        getline(ss, database_name, ',');
        getline(ss, date_created, ',');
        cout << "| " << setw(20) << left << database_name << " | " << setw(19) << left << date_created << " |" << endl;
    }
    cout << "+----------------------+---------------------+" << endl;
    file.close();
}

void Database::displayTables() const
{
    // Calculate maximum width needed
    size_t maxWidth = max(string("Tables in " + name).size(), size_t(15)); // Minimum width of 15
    for (const auto &table : tables)
    {
        maxWidth = max(maxWidth, table.size() + 2); // +2 for "| " prefix
    }
    maxWidth = maxWidth + 2; // Add space for right padding and border

    // Top border
    cout << "+" << string(maxWidth, '-') << "+" << endl;

    // Header
    string header = "Tables in " + name;
    size_t headerPadding = maxWidth - header.size() - 2; // -2 for borders
    cout << "| " << header << string(headerPadding, ' ') << " |" << endl;

    // Separator
    cout << "+" << string(maxWidth, '-') << "+" << endl;

    // Table names or empty message
    if (tables.empty())
    {
        string emptyMsg = "No tables";
        size_t emptyPadding = maxWidth - emptyMsg.size() - 2;
        cout << "| " << emptyMsg << string(emptyPadding, ' ') << " |" << endl;
    }
    else
    {
        for (const auto &table : tables)
        {
            size_t padding = maxWidth - table.size() - 2; // -2 for "| " and " |"
            cout << "| " << table << string(padding, ' ') << " |" << endl;
        }
    }

    // Bottom border
    cout << "+" << string(maxWidth, '-') << "+" << endl;
}

void Database::drop(const string &tableName)
{
    if (!tableExists(tableName))
    {
        cerr << RED << "Table does not exist" << RESET << endl;
        return;
    }

    tables.erase(remove(tables.begin(), tables.end(), tableName), tables.end());
    // Correct the tables.csv file
    string tablesFile = "./Databases/" + name + "/tables.csv";
    ifstream file(tablesFile);
    if (!file.is_open())
    {
        cerr << RED << "Failed to open " << tablesFile << RESET << endl;
        return;
    }

    string line;
    vector<string> lines; // Store all lines except the one to be removed
    while (getline(file, line))
    {
        if (line.find(tableName) == string::npos)
        {
            lines.push_back(line);
        }
    }
    file.close();

    ofstream tempFile(tablesFile);
    if (!tempFile.is_open())
    {
        cerr << RED << "Failed to open " << tablesFile << " for writing" << RESET << endl;
        return;
    }

    for (const auto &l : lines)
    {
        tempFile << l << endl;
    }
    tempFile.close();
}

bool Database::renameTable(const string &oldName, const string &newName)
{
    if (oldName.empty() || newName.empty())
    {
        cerr << RED << "Table names cannot be empty" << RESET << endl;
        return false;
    }

    if (!tableExists(oldName))
    {
        cerr << RED << "Table does not exist: " << oldName << RESET << endl;
        return false;
    }

    if (tableExists(newName))
    {
        cerr << RED << "Table already exists: " << newName << RESET << endl;
        return false;
    }

    string oldPath = "./Databases/" + name + "/" + oldName;
    string newPath = "./Databases/" + name + "/" + newName;
    if (rename(oldPath.c_str(), newPath.c_str()) == 0)
    {
        tables.erase(remove(tables.begin(), tables.end(), oldName), tables.end());
        tables.push_back(newName);
        //fix the tables.csv file
        string tablesFile = "./Databases/" + name + "/tables.csv";
        ifstream file(tablesFile);
        if (!file.is_open())
        {
            cerr << RED << "Failed to open " << tablesFile << RESET << endl;
            return false;
        }

        string line;
        ofstream tempFile("./Databases/" + name + "/temp.csv");
        while (getline(file, line))
        {
            if (line.find(oldName) == string::npos)
            {
                tempFile << line << endl;
            }
            else
            {
                tempFile << newName << "," << currentDateTime() << endl;
            }
        }
        file.close();
        tempFile.close();

        if (remove(tablesFile.c_str()) != 0)
        {
            cerr << RED << "Failed to remove " << tablesFile << RESET << endl;
            return false;
        }
        if (rename(("./Databases/" + name + "/temp.csv").c_str(), tablesFile.c_str()) != 0)
        {
            cerr << RED << "Failed to rename temp.csv to " << tablesFile << RESET << endl;
            return false;
        }
        return true;
    }
    else
    {
        cerr << RED << "Failed to rename table " << oldName << " to " << newName << RESET << endl;
        return false;
    }
}
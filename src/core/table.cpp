#include <iostream>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <string>
#include <ctime>      // For currentDateTime()
#include "database.h" // Include the header for the Database class
#include "globals.h"
#include "table.h"
using namespace std;

// Constructor to initialize columns
Table::Table() : db(*(new Database())), tableName("") {}

Table::Table(Database &db, string tableName, const vector<string> &columnName, const vector<string> &type)
    : db(db), tableName(tableName)
{
    if (tableName.empty())
    {
        cerr << RED << "Table Name cannot be empty" << RESET << endl;
        return;
    }

    string tablePath = "./Databases/" + db.getName() + "/" + tableName;

    if (db.tableExists(tableName))
    {
        // cout << ORANGE << "Table already exists! Loading columns..." << RESET << endl;
        ifstream columnFile(tablePath + "/columns.csv");

        if (!columnFile.is_open())
        {
            cerr << RED << "Failed to open columns.csv for table " << tableName << RESET << endl;
            return;
        }

        string line, colName, dataType;
        int index = 0;
        while (getline(columnFile, line))
        {
            stringstream ss(line);
            getline(ss, colName, ',');
            getline(ss, dataType, ',');
            columns[colName] = {index++, datatype[dataType]};
        }
        columnFile.close();
        return;
    }

    // Create the table directory if it doesn't exist
    if (!filesystem::exists(tablePath) && !filesystem::create_directories(tablePath))
    {
        throw runtime_error("Failed to create table directory");
    }

    // Ensure columnName and type vectors are valid
    if (columnName.size() != type.size())
    {
        cerr << RED << "Column names and types size mismatch!" << RESET << endl;
        return;
    }

    ofstream columnFile(tablePath + "/columns.csv");
    if (!columnFile.is_open())
    {
        cerr << RED << "Failed to create columns.csv for table " << tableName << RESET << endl;
        return;
    }

    for (size_t i = 0; i < columnName.size(); i++)
    {
        columns[columnName[i]] = {i, datatype[type[i]]};
        columnFile << columnName[i] << "," << type[i] << endl;
    }
    columnFile.close();

    ofstream dataFile(tablePath + "/data.csv");
    dataFile.close();

    ofstream tableFile("./Databases/" + db.getName() + "/tables.csv", ios::app);
    tableFile << tableName << "," << currentDateTime() << endl;
    tableFile.close();

    cout << GREEN << "Table " << tableName << " created successfully." << RESET << endl;
}

Table selectTable(Database &db, const string &tableName)
{
    if (tableName.empty())
    {
        cerr << RED << "Table name cannot be empty" << RESET << endl;
        return Table();
    }

    if (!db.tableExists(tableName))
    {
        cerr << RED << "Table does not exist: " << tableName << RESET << endl;
        return Table();
    }

    cout << GREEN << "Loading table: " << tableName << RESET << endl;

    string tablePath = "./Databases/" + db.getName() + "/" + tableName + "/columns.csv";
    ifstream columnFile(tablePath);
    if (!columnFile.is_open())
    {
        cerr << RED << "Failed to open columns.csv for table " << tableName << RESET << endl;
        return Table();
    }

    vector<string> columnNames, dataTypes;
    string line, colName, dataType;
    while (getline(columnFile, line))
    {
        stringstream ss(line);
        getline(ss, colName, ',');
        getline(ss, dataType, ',');

        if (colName.empty() || dataType.empty())
        {
            cerr << ORANGE << "Skipping invalid column entry: " << line << RESET << endl;
            continue;
        }

        columnNames.push_back(colName);
        dataTypes.push_back(dataType);
    }
    columnFile.close();

    return Table(db, tableName, columnNames, dataTypes);
}


void Table::insert(const vector<string> &rowData)
{
    // If columns are not yet loaded, read from columns.csv
    if (columns.empty())
    {
        string filePath = "./Databases/" + db.getName() + "/" + tableName + "/columns.csv";
        ifstream colFile(filePath);
        if (!colFile.is_open())
        {
            cerr << RED << "Failed to open " << filePath << " for table " << tableName << RESET << endl;
            return;
        }

        string line;
        int index = 0;
        while (getline(colFile, line))
        {
            stringstream ss(line);
            string colName, colTypeStr;
            getline(ss, colName, ',');
            getline(ss, colTypeStr, ',');

            // Map data type string to internal type ID
            auto it = datatype.find(colTypeStr);
            if (it == datatype.end())
            {
                cerr << RED << "Unknown data type '" << colTypeStr << "' for column '" << colName << "' in " << tableName << RESET << endl;
                colFile.close();
                return;
            }
            int colType = it->second;

            columns[colName] = {index++, colType}; // Store name -> (index, type)
        }
        colFile.close();

        if (columns.empty())
        {
            cerr << RED << "No columns defined in " << filePath << " for table " << tableName << RESET << endl;
            return;
        }
    }

    if (rowData.size() != columns.size())
    {
        cerr << RED << "Row size mismatch! Expected " << columns.size() << " columns, got " << rowData.size() << RESET << endl;
        return;
    }

    if (!db.isValid())
    {
        cerr << RED << "Database reference is missing!" << RESET << endl;
        return;
    }

    // Validate data types against schema order
    vector<pair<string, pair<int, int>>> sortedColumns(columns.begin(), columns.end());
    sort(sortedColumns.begin(), sortedColumns.end(), 
         [](const auto& a, const auto& b) { return a.second.first < b.second.first; });

    for (size_t i = 0; i < rowData.size(); i++)
    {
        const string &colName = sortedColumns[i].first;
        int colType = sortedColumns[i].second.second; // Type ID
        string value = rowData[i];

        // Allow NULL values without validation
        if (value == "NULL")
        {
            continue;
        }

        bool isValid = true;
        switch (colType)
        {
        case 0: // INT
            try { stoi(value); } catch (...) { isValid = false; }
            break;
        case 1: // FLOAT
            try { stof(value); } catch (...) { isValid = false; }
            break;
        case 2: // BOOL or BOOLEAN
            if (value != "TRUE" && value != "FALSE" && value != "1" && value != "0")
                isValid = false;
            break;
        case 3: // STRING
            if (value.front() == '\'' && value.back() == '\'')
                value = value.substr(1, value.size() - 2);
            break;
        case 4: // DATE
            if (value.size() != 10 || value[4] != '-' || value[7] != '-' || !isdigit(value[0]) || 
                !isdigit(value[1]) || !isdigit(value[2]) || !isdigit(value[3]) || !isdigit(value[5]) || 
                !isdigit(value[6]) || !isdigit(value[8]) || !isdigit(value[9]))
                isValid = false;
            break;
        default: // Handle unknown data types
            cerr << RED << "Error: Unsupported data type for column '" << colName << "'." << RESET << endl;
            return;
        }

        if (!isValid)
        {
            // Find the type name for error reporting
            string typeName;
            for (const auto& [name, id] : datatype)
            {
                if (id == colType) { typeName = name; break; }
            }
            cerr << RED << "Error: Invalid value '" << value << "' for column '" << colName
                 << "' (Expected " << typeName << ")." << RESET << endl;
            return;
        }
    }

    // Write data to file
    string dataFilePath = "./Databases/" + db.getName() + "/" + tableName + "/data.csv";
    ofstream dataFile(dataFilePath, ios::app);
    if (!dataFile.is_open())
    {
        cerr << RED << "Failed to open " << dataFilePath << " for table " << tableName << RESET << endl;
        return;
    }

    for (size_t i = 0; i < rowData.size(); i++)
    {
        dataFile << rowData[i];
        if (i < rowData.size() - 1)
            dataFile << ",";
    }
    dataFile << endl;
    dataFile.close();

    cout << GREEN << "Row added successfully to table " << tableName << "." << RESET << endl;
}

void Table::insertWithColumns(const vector<string> &columnNames, const vector<string> &rowData)
{
    if (columns.empty())
    {
        cerr << RED << "Table is not loaded correctly! No columns found." << RESET << endl;
        return;
    }

    if (columnNames.size() != rowData.size())
    {
        cerr << RED << "Mismatch between provided column names and values!" << RESET << endl;
        return;
    }

    if (!db.isValid())
    {
        cerr << RED << "Database reference is missing!" << RESET << endl;
        return;
    }

    // Validate column names
    for (const string &colName : columnNames)
    {
        if (columns.find(colName) == columns.end())
        {
            cerr << RED << "Error: Column '" << colName << "' does not exist in table '" << tableName << "'!" << RESET << endl;
            return;
        }
    }

    // Validate data types
    unordered_map<string, string> providedValues;
    for (size_t i = 0; i < columnNames.size(); i++)
    {
        string colName = columnNames[i];
        int colType = columns[colName].second;
        if (datatypeName.find(colType) == datatypeName.end())
        {
            cerr << RED << "Error: Unknown datatype ID '" << colType << "' for column '" << colName << "'!" << RESET << endl;
            return;
        }

        string value = rowData[i];

        // Allow NULL values without validation
        if (value == "NULL")
        {
            providedValues[colName] = value;
            continue;
        }

        // Check if datatype is valid
        string typeStr = datatypeName[colType];
        if (datatype.find(typeStr) == datatype.end())
        {
            cerr << RED << "Error: Unknown datatype '" << colType << "' for column '" << colName << "'!" << RESET << endl;
            return;
        }
        int expectedType = datatype[typeStr];

        // Type validation
        bool isValid = true;
        switch (expectedType)
        {
        case 0: // INT
            try
            {
                stoi(value);
            }
            catch (...)
            {
                isValid = false;
            }
            break;
        case 1: // FLOAT
            try
            {
                stof(value);
            }
            catch (...)
            {
                isValid = false;
            }
            break;
        case 2: // BOOL
            if (value != "TRUE" && value != "FALSE" && value != "1" && value != "0")
            {
                isValid = false;
            }
            break;
        case 3: // STRING
            if (value.front() == '\'' && value.back() == '\'')
            {
                value = value.substr(1, value.size() - 2); // Remove quotes
            }
            break;
        case 4: // DATE (YYYY-MM-DD format check)
            if (value.size() != 10 || value[4] != '-' || value[7] != '-' ||
                !isdigit(value[0]) || !isdigit(value[1]) || !isdigit(value[2]) || !isdigit(value[3]) ||
                !isdigit(value[5]) || !isdigit(value[6]) ||
                !isdigit(value[8]) || !isdigit(value[9]))
            {
                isValid = false;
            }
            break;
        }

        if (!isValid)
        {
            cerr << RED << "Error: Invalid value '" << value << "' for column '" << colName
                 << "' (Expected " << datatypeName[expectedType] << ")." << RESET << endl;
            return;
        }

        providedValues[colName] = value;
    }

    // Fill fullRow based on actual column order
    vector<string> fullRow(columns.size(), "NULL");

    // Iterate through columns in the correct schema order
    for (const auto &col : columns)
    {
        const string &colName = col.first;
        int colIndex = col.second.first; // Correct column index

        if (providedValues.find(colName) != providedValues.end())
        {
            fullRow[colIndex] = providedValues[colName];
        }
    }

    // Open file to append
    string filePath = "./Databases/" + db.getName() + "/" + tableName + "/data.csv";
    ofstream dataFile(filePath, ios::app);
    if (!dataFile.is_open())
    {
        cerr << RED << "Failed to open " << filePath << " for table " << tableName << RESET << endl;
        return;
    }

    // Write row to file
    for (size_t i = 0; i < fullRow.size(); i++)
    {
        dataFile << fullRow[i];
        if (i < fullRow.size() - 1)
        {
            dataFile << ",";
        }
    }
    dataFile << endl;
    dataFile.close();

    cout << GREEN << "Row added successfully to table " << tableName << "." << RESET << endl;
}

void Table::displayTable(const vector<string>& columnNames = {})
{
    // Sort columns by schema index
    vector<pair<string, pair<int, int>>> sortedColumns(columns.begin(), columns.end());
    sort(sortedColumns.begin(), sortedColumns.end(), 
         [](const auto& a, const auto& b) { return a.second.first < b.second.first; });

    if (sortedColumns.empty())
    {
        cout << ORANGE << "No columns defined for table " << tableName << RESET << endl;
        return;
    }

    // Calculate column widths and store data in memory
    unordered_map<string, size_t> columnWidths;
    vector<vector<string>> rows; // Store all rows for single-pass reading
    for (const auto &col : sortedColumns)
    {
        columnWidths[col.first] = col.first.size(); // Initialize with header size
    }

    string filePath = "./Databases/" + db.getName() + "/" + tableName + "/data.csv";
    ifstream dataFile(filePath);
    if (!dataFile.is_open())
    {
        cerr << RED << "Failed to open data.csv for table " << tableName << RESET << endl;
        return;
    }

    string line;
    while (getline(dataFile, line))
    {
        stringstream ss(line);
        string cell;
        vector<string> row;
        for (const auto &col : sortedColumns)
        {
            if (getline(ss, cell, ','))
            {
                row.push_back(cell);
                columnWidths[col.first] = max(columnWidths[col.first], cell.size());
            }
            else
            {
                row.push_back("NULL"); // Handle missing fields
                columnWidths[col.first] = max(columnWidths[col.first], string("NULL").size());
            }
        }
        rows.push_back(row);
    }
    dataFile.close();

    // Display header
    for (const auto &col : sortedColumns)
    {
        if (columnNames.empty() || find(columnNames.begin(), columnNames.end(), col.first) != columnNames.end())
        {
            cout << "| " << col.first << string(columnWidths[col.first] - col.first.size() + 1, ' ');
        }
    }
    cout << "|\n";

    // Display separator
    for (const auto &col : sortedColumns)
    {
        if (columnNames.empty() || find(columnNames.begin(), columnNames.end(), col.first) != columnNames.end())
        {
            cout << "+" << string(columnWidths[col.first] + 2, '-');
        }
    }
    cout << "+\n";

    // Display data
    if (rows.empty())
    {
        // Calculate total width for a single centered message
        size_t totalWidth = 0;
        for (const auto &col : sortedColumns)
        {
            if (columnNames.empty() || find(columnNames.begin(), columnNames.end(), col.first) != columnNames.end())
            {
                totalWidth += columnWidths[col.first] + 3; // +2 for padding, +1 for separator
            }
        }
        totalWidth -= 1; // Remove extra separator at the end

        string message = "No data in table " + tableName;
        size_t padding = totalWidth > message.size() ? (totalWidth - message.size()) / 2 : 0;
        cout << "|" << string(padding, ' ') << message << string(totalWidth - message.size() - padding, ' ') << "|\n";
    }
    else
    {
        for (const auto &row : rows)
        {
            for (size_t i = 0; i < sortedColumns.size(); ++i)
            {
                const string &cell = row[i];
                if (columnNames.empty() || find(columnNames.begin(), columnNames.end(), sortedColumns[i].first) != columnNames.end())
                {
                    cout << "| " << cell << string(columnWidths[sortedColumns[i].first] - cell.size() + 1, ' ');
                }
            }
            cout << "|\n";
        }
    }

    // Display bottom separator
    for (const auto &col : sortedColumns)
    {
        if (columnNames.empty() || find(columnNames.begin(), columnNames.end(), col.first) != columnNames.end())
        {
            cout << "+" << string(columnWidths[col.first] + 2, '-');
        }
    }
    cout << "+\n";
}

void create(Database &db, const string &tableName, const vector<string> &columns, const vector<string> &datatypes)
{
    if (tableName.empty())
    {
        cerr << RED << "Table name cannot be empty" << RESET << endl;
        return;
    }

    if (db.tableExists(tableName))
    {
        cerr << ORANGE << "Table already exists!" << RESET << endl;
        return;
    }

    if (columns.size() != datatypes.size())
    {
        cerr << RED << "Columns and datatypes size mismatch!" << RESET << endl;
        return;
    }

    for (const auto &dt : datatypes)
    {
        if (datatype.find(dt) == datatype.end())
        {
            cerr << RED << "Invalid datatype: " << dt << RESET << endl;
            return;
        }
    }

    string tablePath = "./Databases/" + db.getName() + "/" + tableName;
    filesystem::create_directories(tablePath);

    ofstream columnFile(tablePath + "/columns.csv");
    for (size_t i = 0; i < columns.size(); i++)
    {
        columnFile << columns[i] << "," << datatypes[i] << endl;
    }
    columnFile.close();

    ofstream("./Databases/" + db.getName() + "/" + tableName + "/data.csv").close();
    ofstream("./Databases/" + db.getName() + "/tables.csv", ios::app) << tableName << "," << currentDateTime() << endl;
    db.addTable(tableName);
    cout << GREEN << "Table " << tableName << " created successfully." << RESET << endl;
}

void rename(Database& db, const string& oldName, const string& newName) {
    if (oldName.empty() || newName.empty()) {
        cerr << RED << "Table names cannot be empty" << RESET << endl;
        return;
    }

    if (!db.tableExists(oldName)) {
        cerr << RED << "Table does not exist: " << oldName << RESET << endl;
        return;
    }

    if (db.tableExists(newName)) {
        cerr << RED << "Table already exists: " << newName << RESET << endl;
        return;
    }

    string oldPath = "./Databases/" + db.getName() + "/" + oldName;
    string newPath = "./Databases/" + db.getName() + "/" + newName;
    if(db.renameTable(oldName, newName)) {
        filesystem::rename(oldPath,newPath);
        cout << GREEN << "Table " << oldName << " renamed to " << newName << RESET << endl;
    } else {
        cerr << RED << "Failed to rename table " << oldName << " to " << newName << RESET << endl;
    }
}

void drop(Database& db, const string& tableName) {
    if (!db.tableExists(tableName)) {
        cerr << RED << "Table does not exist" << RESET << endl;
        return;
    }

    string tablePath = "./Databases/" + db.getName() + "/" + tableName;
    if(filesystem::remove_all(tablePath)) {
        cout << GREEN << "Dropping table " << tableName << "..." << RESET << endl;
        db.drop(tableName);
        cout << GREEN << "Table " << tableName << " dropped successfully." << RESET << endl;
    } else {
        cerr << RED << "Failed to drop table " << tableName << RESET << endl;
    }
}

void truncate(Database& db, const string& tableName) {
    if (!db.tableExists(tableName)) {
        cerr << RED << "Table does not exist" << RESET << endl;
        return;
    }

    string tablePath = "./Databases/" + db.getName() + "/" + tableName + "/data.csv";
    ofstream dataFile(tablePath);
    dataFile.close();
    cout << GREEN << "Table " << tableName << " truncated successfully." << RESET << endl;
}
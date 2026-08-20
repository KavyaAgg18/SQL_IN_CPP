#include "sqlparser.h"
#include "table.h"
#include "join.h"
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include "globals.h"

using namespace std;

// Helper function to convert a string to uppercase
string toUpperCase(string str)
{
    transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

// Helper function to convert a string to lowercase (for table/column names)
string toLowerCase(string str)
{
    transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

void SQLParser::executeQuery(Database &db, const string &query)
{
    stringstream ss(query);
    string command;
    ss >> command;
    command = toUpperCase(command); // Convert command to uppercase

    if (command == "CREATE")
    {
        string temp, tableName;
        ss >> temp;
        temp = toUpperCase(temp);
        if (temp != "TABLE")
        {
            cerr << "Syntax error: Expected 'TABLE' after CREATE\n";
            return;
        }

        ss >> tableName;
        tableName = toLowerCase(tableName);

        string columnsDefinition;
        getline(ss, columnsDefinition, ')');
        columnsDefinition.erase(0, columnsDefinition.find('(') + 1);

        stringstream colStream(columnsDefinition);
        vector<string> columnNames, columnTypes;
        string column, type;

        while (getline(colStream, column, ','))
        {
            stringstream columnStream(column);
            columnStream >> column >> type;
            column = toLowerCase(column);
            type = toUpperCase(type);

            if (column.empty() || type.empty())
            {
                cerr << "Invalid column definition: " << column << " " << type << "\n";
                return;
            }
            columnNames.push_back(column);
            columnTypes.push_back(type);
        }

        create(db, tableName, columnNames, columnTypes);
    }
    else if (command == "INSERT")
    {
        string temp, tableName;
        ss >> temp;
        temp = toUpperCase(temp);

        if (temp != "INTO")
        {
            cerr << "Syntax error: Expected 'INTO' after INSERT\n";
            return;
        }

        ss >> tableName;
        tableName = toLowerCase(tableName);

        string rowDataStr;
        getline(ss, rowDataStr);

        size_t openParen = rowDataStr.find('(');
        size_t closeParen = rowDataStr.find(')');

        if (openParen == string::npos || closeParen == string::npos)
        {
            cerr << "Syntax error: Missing parentheses in INSERT statement\n";
            return;
        }

        vector<string> columnNames;
        vector<string> rowData;

        bool hasColumns = (rowDataStr.find("VALUES") != string::npos);
        size_t valuesPos = rowDataStr.find("VALUES");

        if (hasColumns)
        {
            string colNamesStr = rowDataStr.substr(0, valuesPos);
            colNamesStr = colNamesStr.substr(colNamesStr.find('(') + 1, colNamesStr.find(')') - colNamesStr.find('(') - 1);

            stringstream colStream(colNamesStr);
            string colName;
            while (getline(colStream, colName, ','))
            {
                colName.erase(0, colName.find_first_not_of(" \t"));
                colName.erase(colName.find_last_not_of(" \t") + 1);
                columnNames.push_back(colName);
            }

            rowDataStr = rowDataStr.substr(valuesPos + 6);
            openParen = rowDataStr.find('(');
            closeParen = rowDataStr.find(')');
            rowDataStr = rowDataStr.substr(openParen + 1, closeParen - openParen - 1);
        }
        else
        {
            rowDataStr = rowDataStr.substr(openParen + 1, closeParen - openParen - 1);
        }

        stringstream rowStream(rowDataStr);
        string value;
        while (getline(rowStream, value, ','))
        {
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            if (!value.empty() && value.front() == '\'' && value.back() == '\'')
            {
                value = value.substr(1, value.size() - 2);
            }
            rowData.push_back(value);
        }

        Table table = selectTable(db, tableName);

        if (hasColumns)
        {
            table.insertWithColumns(columnNames, rowData);
        }
        else
        {
            table.insert(rowData);
        }
    }
    else if (command == "SELECT")
    {
        vector<string> columnNames;
        string temp, tableName;

        // Parse column names or '*' until 'FROM'
        while (ss >> temp)
        {
            temp = toUpperCase(temp);
            if (temp == "FROM")
            {
                break; // Stop collecting column names
            }
            if (temp == "*")
            {
                columnNames.clear(); // Empty vector means all columns
                continue;
            }
            if (temp.back() == ',') // Handle trailing comma
            {
                temp.pop_back();
            }
            columnNames.push_back(toLowerCase(temp));
        }
        // Get table name
        ss >> tableName;
        tableName = toLowerCase(tableName);

        // Check for JOIN clause
        ss >> temp;
        temp = toUpperCase(temp);

        if (temp == "INNER" || temp == "LEFT" || temp == "RIGHT" || temp == "FULL")
        {
            string joinType = temp;
            ss >> temp; // Should be "JOIN"
            temp = toUpperCase(temp);
            if (temp != "JOIN")
            {
                cerr << "Syntax error: Expected 'JOIN' after join type\n";
                return;
            }

            string table2Name;
            ss >> table2Name;
            table2Name = toLowerCase(table2Name);

            // Parse ON condition
            ss >> temp; // Should be "ON"
            temp = toUpperCase(temp);
            if (temp != "ON")
            {
                cerr << "Syntax error: Expected 'ON' after join table\n";
                return;
            }

            vector<string> joinColumns;
            string joinColumn1, joinColumn2;
            ss >> joinColumn1 >> temp >> joinColumn2;

            // Remove trailing semicolon if present
            if (!joinColumn1.empty() && joinColumn1.back() == ';')
            {
                joinColumn1.pop_back();
            }
            if (!joinColumn2.empty() && joinColumn2.back() == ';')
            {
                joinColumn2.pop_back();
            }

            joinColumns.push_back(joinColumn1);
            joinColumns.push_back(joinColumn2);
            cout << "working fine till here " << joinType << " " << tableName << " " << table2Name << " " << joinColumns[0] << " " << joinColumns[1] << endl;
            Table table1 = selectTable(db, tableName);
            Table table2 = selectTable(db, table2Name);
            if (joinType == "INNER")
            {
                innerJoin(db, table1, table2, joinColumns);
            }
            
            else if (joinType == "RIGHT")
            {
                rightJoin(db, table1, table2, joinColumns);
            }
            else if (joinType == "LEFT")
            {
                leftJoin(db, table1, table2, joinColumns);
            }
            else if (joinType == "FULL")
            {
                fullJoin(db, table1, table2, joinColumns);
            }
            // else if(joinType == "CROSS")
            // {
            //     crossJoin(db, table1, table2);
            // }
            else
            {
                cerr << "Invalid JOIN type: " << joinType << "\n";
                return;
            }
            return;
        }

        // If no JOIN, proceed with normal SELECT
        if (!tableName.empty() && tableName.back() == ';')
        {
            tableName.pop_back();
        }

        if (tableName.empty())
        {
            cerr << "Syntax error: Missing table name after 'FROM'\n";
            return;
        }

        Table table = selectTable(db, tableName);

        if (table.getName().empty())
        {
            cerr << "Table '" << tableName << "' does not exist!\n";
            return;
        }

        table.displayTable(columnNames);
    }
    else if (command == "RENAME")
    {
        string temp, oldTableName, newTableName;
        ss >> temp;
        temp = toUpperCase(temp);

        if (temp != "TABLE")
        {
            cerr << "Syntax error: Expected 'TABLE' after RENAME\n";
            return;
        }

        ss >> oldTableName;
        oldTableName = toLowerCase(oldTableName);

        ss >> temp;
        temp = toUpperCase(temp);
        if (temp != "TO")
        {
            cerr << "Syntax error: Expected 'TO' after table name in RENAME\n";
            return;
        }

        ss >> newTableName;
        newTableName = toLowerCase(newTableName);

        if (!newTableName.empty() && newTableName.back() == ';')
        {
            newTableName.pop_back();
        }

        if (oldTableName.empty() || newTableName.empty())
        {
            cerr << "Syntax error: Missing table names in RENAME statement\n";
            return;
        }

        rename(db, oldTableName, newTableName);
    }
    else if (command == "DROP")
    {
        string temp, tableName;
        ss >> temp;
        temp = toUpperCase(temp);

        if (temp != "TABLE")
        {
            cerr << "Syntax error: Expected 'TABLE' after DROP\n";
            return;
        }

        ss >> tableName;
        tableName = toLowerCase(tableName);

        if (!tableName.empty() && tableName.back() == ';')
        {
            tableName.pop_back();
        }

        if (tableName.empty())
        {
            cerr << "Syntax error: Missing table name after 'TABLE' in DROP\n";
            return;
        }

        drop(db, tableName);
    }
    else if (command == "TRUNCATE")
    {
        string temp, tableName;
        ss >> temp;
        temp = toUpperCase(temp);

        if (temp != "TABLE")
        {
            cerr << "Syntax error: Expected 'TABLE' after TRUNCATE\n";
            return;
        }

        ss >> tableName;
        tableName = toLowerCase(tableName);

        if (!tableName.empty() && tableName.back() == ';')
        {
            tableName.pop_back();
        }

        if (tableName.empty())
        {
            cerr << "Syntax error: Missing table name after 'TABLE' in TRUNCATE\n";
            return;
        }

        truncate(db, tableName);
    }
    else if(command=="SHOW"){
        string temp;
        ss >> temp;
        temp = toUpperCase(temp);

        // Strip whitespace from both ends
        temp.erase(0, temp.find_first_not_of(" \t\n\r"));
        temp.erase(temp.find_last_not_of(" \t\n\r") + 1);

        // Remove trailing semicolon if present
        if (!temp.empty() && temp.back() == ';') {
            temp.pop_back();
        }

        printf("temp: %s\n",temp.c_str());
        if (temp != "TABLES" && temp != "DATABASES")
        {
            cerr << "Syntax error: Expected 'TABLES' or 'DATABASES' after SHOW\n";
            return;
        }
        if (temp == "DATABASES")
        {
            displayDatabases();
            return;
        }
        if (temp == "TABLES")
        {
            db.displayTables();
            return;
        }
    }
    else
    {
        cerr << "Invalid SQL Query!\n";
    }
}
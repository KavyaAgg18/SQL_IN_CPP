#include <iostream>
#include <fstream>
#include <vector>
#include "join.h"
#include <unordered_map>
#include <string>
#include <sstream>
#include <filesystem>
#include "table.h"
#include "database.h"
#include "globals.h"

using namespace std;

// INNER JOIN
void innerJoin(Database &db, Table &table1, Table &table2, const vector<string> &joinColumns)
{
    // Validate join columns
    if (joinColumns.size() < 2)
    {
        cerr << "Error: Need exactly two columns for join condition (table1.col and table2.col)" << endl;
        return;
    }

    // Check if temp_joined_table exists and drop it if necessary
    if (db.tableExists("temp_joined_table"))
    {
        cout << "Cleaning up existing temp_joined_table..." << endl;
        truncate(db, "temp_joined_table");
    }

    // Extract column names and types from both tables
    vector<string> table1Columns, table2Columns;
    vector<string> table1Types, table2Types;

    for (const auto &pair : table1.columns)
    {
        table1Columns.push_back(pair.first);
        table1Types.push_back(datatypeName[table1.columns[pair.first].second]);
    }
    for (const auto &pair : table2.columns)
    {
        table2Columns.push_back(pair.first);
        table2Types.push_back(datatypeName[table2.columns[pair.first].second]);
    }

    // Verify join columns exist in their respective tables
    string table1JoinCol = joinColumns[0];
    string table2JoinCol = joinColumns[1];

    if (table1.columns.find(table1JoinCol) == table1.columns.end())
    {
        cerr << "Error: Join column '" << table1JoinCol << "' not found in table " << table1.getName() << endl;
        return;
    }

    if (table2.columns.find(table2JoinCol) == table2.columns.end())
    {
        cerr << "Error: Join column '" << table2JoinCol << "' not found in table " << table2.getName() << endl;
        return;
    }

    // Get indices of join columns
    int table1JoinIdx = table1.columns[table1JoinCol].first;
    int table2JoinIdx = table2.columns[table2JoinCol].first;

    // Create joined columns and their types
    vector<string> joinedColumns = table1Columns;
    vector<string> joinedTypes = table1Types;

    for (size_t i = 0; i < table2Columns.size(); ++i)
    {
        if (table2Columns[i] != table2JoinCol)
        {
            joinedColumns.push_back(table2Columns[i]);
            joinedTypes.push_back(table2Types[i]);
        }
    }

    // Create a temporary table with proper column types
    Table joinedTable(db, "temp_joined_table", joinedColumns, joinedTypes);
    db.addTable("temp_joined_table");
    // Perform the inner join and populate the temporary table
    ifstream table1File("./Databases/" + db.getName() + "/" + table1.getName() + "/data.csv");
    ifstream table2File("./Databases/" + db.getName() + "/" + table2.getName() + "/data.csv");

    if (!table1File.is_open() || !table2File.is_open())
    {
        cerr << "Error: Unable to open table data files." << endl;
        return;
    }

    string row1Line, row2Line;
    while (getline(table1File, row1Line))
    {
        stringstream row1Stream(row1Line);
        vector<string> row1;
        string cell;
        while (getline(row1Stream, cell, ','))
        {
            row1.push_back(cell);
        }

        // Reset table2File to the beginning for each row1
        table2File.clear();
        table2File.seekg(0, ios::beg);

        while (getline(table2File, row2Line))
        {
            stringstream row2Stream(row2Line);
            vector<string> row2;
            while (getline(row2Stream, cell, ','))
            {
                row2.push_back(cell);
            }

            if (row1[table1JoinIdx] == row2[table2JoinIdx])
            {
                vector<string> combinedRow = row1;
                for (size_t i = 0; i < row2.size(); ++i)
                {
                    if (i != static_cast<size_t>(table2JoinIdx))
                    {
                        combinedRow.push_back(row2[i]);
                    }
                }
                try
                {
                    joinedTable.insert(combinedRow);
                }
                catch (const exception &e)
                {
                    cerr << "Error inserting row: " << e.what() << endl;
                }
            }
        }
    }

    table1File.close();
    table2File.close();

    // Display the results
    cout << "Inner Join Result:" << endl;
    joinedTable.displayTable(vector<string>());
    // Delete the temporary table
    drop(db, "temp_joined_table");
}
void rightJoin(Database &db, Table &table1, Table &table2, const vector<string> &joinColumns)
{
    // Validate join columns
    if (joinColumns.size() < 2)
    {
        cerr << "Error: Need exactly two columns for join condition (table1.col and table2.col)" << endl;
        return;
    }

    // Check if temp_joined_table exists and drop it if necessary
    if (db.tableExists("temp_joined_table"))
    {
        cout << "Cleaning up existing temp_joined_table..." << endl;
        truncate(db, "temp_joined_table");
    }

    // Extract column names and types from both tables
    vector<string> table1Columns, table2Columns;
    vector<string> table1Types, table2Types;

    for (const auto &pair : table1.columns)
    {
        table1Columns.push_back(pair.first);
        table1Types.push_back(datatypeName[table1.columns[pair.first].second]);
    }
    for (const auto &pair : table2.columns)
    {
        table2Columns.push_back(pair.first);
        table2Types.push_back(datatypeName[table2.columns[pair.first].second]);
    }

    // Verify join columns exist
    string table1JoinCol = joinColumns[0];
    string table2JoinCol = joinColumns[1];

    if (table1.columns.find(table1JoinCol) == table1.columns.end())
    {
        cerr << "Error: Join column '" << table1JoinCol << "' not found in table " << table1.getName() << endl;
        return;
    }

    if (table2.columns.find(table2JoinCol) == table2.columns.end())
    {
        cerr << "Error: Join column '" << table2JoinCol << "' not found in table " << table2.getName() << endl;
        return;
    }

    // Get join column indices
    int table1JoinIdx = table1.columns[table1JoinCol].first;
    int table2JoinIdx = table2.columns[table2JoinCol].first;

    // Create combined column names and types for the result
    vector<string> joinedColumns = table1Columns;
    vector<string> joinedTypes = table1Types;

    for (size_t i = 0; i < table2Columns.size(); ++i)
    {
        if (table2Columns[i] != table2JoinCol)
        {
            joinedColumns.push_back(table2Columns[i]);
            joinedTypes.push_back(table2Types[i]);
        }
        
    }

    // Create a temporary result table
    Table joinedTable(db, "temp_joined_table", joinedColumns, joinedTypes);
    db.addTable("temp_joined_table");

    // Open both data files
    ifstream table1File("./Databases/" + db.getName() + "/" + table1.getName() + "/data.csv");
    ifstream table2File("./Databases/" + db.getName() + "/" + table2.getName() + "/data.csv");

    if (!table1File.is_open() || !table2File.is_open())
    {
        cerr << "Error: Unable to open table data files." << endl;
        return;
    }

    string row2Line, row1Line;
    while (getline(table2File, row2Line))
    {
        stringstream row2Stream(row2Line);
        vector<string> row2;
        string cell;
        while (getline(row2Stream, cell, ','))
        {
            row2.push_back(cell);
        }

        bool matched = false;

        // Reset table1File to beginning
        table1File.clear();
        table1File.seekg(0, ios::beg);

        while (getline(table1File, row1Line))
        {
            stringstream row1Stream(row1Line);
            vector<string> row1;
            while (getline(row1Stream, cell, ','))
            {
                row1.push_back(cell);
            }

            if (row1[table1JoinIdx] == row2[table2JoinIdx])
            {
                matched = true;
                vector<string> combinedRow = row1;
                for (size_t i = 0; i < row2.size(); ++i)
                {
                    if (i != static_cast<size_t>(table2JoinIdx))
                    {
                        combinedRow.push_back(row2[i]);
                    }
                }
                try
                {
                    joinedTable.insert(combinedRow);
                }
                catch (const exception &e)
                {
                    cerr << "Error inserting row: " << e.what() << endl;
                }
            }
        }

        if (!matched)
        {
            // Fill left (table1) columns with "null"
            vector<string> nullRow(table1Columns.size(), "NULL");
            for (size_t i = 0; i < row2.size(); ++i)
            {
                if (i != static_cast<size_t>(table2JoinIdx))
                {
                    nullRow.push_back(row2[i]);
                }
            }
            try
            {
                joinedTable.insert(nullRow);
            }
            catch (const exception &e)
            {
                cerr << "Error inserting unmatched row: " << e.what() << endl;
            }
        }
    }

    table1File.close();
    table2File.close();

    // Display the results
    cout << "Right Join Result:" << endl;
    joinedTable.displayTable(vector<string>());

    // Delete the temporary table
    drop(db, "temp_joined_table");
}


void leftJoin(Database &db, Table &table1, Table &table2, const vector<string> &joinColumns)
{
    if (joinColumns.size() < 2)
    {
        cerr << "Error: Need exactly two columns for join condition (table1.col and table2.col)" << endl;
        return;
    }

    if (db.tableExists("temp_joined_table"))
    {
        cout << "Cleaning up existing temp_joined_table..." << endl;
        truncate(db, "temp_joined_table");
    }

    vector<string> table1Columns, table2Columns;
    vector<string> table1Types, table2Types;

    for (const auto &pair : table1.columns)
    {
        table1Columns.push_back(pair.first);
        table1Types.push_back(datatypeName[table1.columns[pair.first].second]);
    }
    for (const auto &pair : table2.columns)
    {
        table2Columns.push_back(pair.first);
        table2Types.push_back(datatypeName[table2.columns[pair.first].second]);
    }

    string table1JoinCol = joinColumns[0];
    string table2JoinCol = joinColumns[1];

    if (table1.columns.find(table1JoinCol) == table1.columns.end())
    {
        cerr << "Error: Join column '" << table1JoinCol << "' not found in table " << table1.getName() << endl;
        return;
    }

    if (table2.columns.find(table2JoinCol) == table2.columns.end())
    {
        cerr << "Error: Join column '" << table2JoinCol << "' not found in table " << table2.getName() << endl;
        return;
    }

    int table1JoinIdx = table1.columns[table1JoinCol].first;
    int table2JoinIdx = table2.columns[table2JoinCol].first;

    vector<string> joinedColumns = table1Columns;
    vector<string> joinedTypes = table1Types;

    for (size_t i = 0; i < table2Columns.size(); ++i)
    {
        if (table2Columns[i] != table2JoinCol)
        {
            joinedColumns.push_back(table2Columns[i]);
            joinedTypes.push_back(table2Types[i]);
        }
    }

    Table joinedTable(db, "temp_joined_table", joinedColumns, joinedTypes);
    db.addTable("temp_joined_table");

    ifstream table1File("./Databases/" + db.getName() + "/" + table1.getName() + "/data.csv");
    ifstream table2File("./Databases/" + db.getName() + "/" + table2.getName() + "/data.csv");

    if (!table1File.is_open() || !table2File.is_open())
    {
        cerr << "Error: Unable to open table data files." << endl;
        return;
    }

    string row1Line, row2Line;
    while (getline(table1File, row1Line))
    {
        stringstream row1Stream(row1Line);
        vector<string> row1;
        string cell;
        while (getline(row1Stream, cell, ','))
        {
            row1.push_back(cell);
        }

        bool matched = false;

        table2File.clear();
        table2File.seekg(0, ios::beg);

        while (getline(table2File, row2Line))
        {
            stringstream row2Stream(row2Line);
            vector<string> row2;
            while (getline(row2Stream, cell, ','))
            {
                row2.push_back(cell);
            }

            if (row1[table1JoinIdx] == row2[table2JoinIdx])
            {
                matched = true;
                vector<string> combinedRow = row1;
                for (size_t i = 0; i < row2.size(); ++i)
                {
                    if (i != static_cast<size_t>(table2JoinIdx))
                    {
                        combinedRow.push_back(row2[i]);
                    }
                }
                try
                {
                    joinedTable.insert(combinedRow);
                }
                catch (const exception &e)
                {
                    cerr << "Error inserting row: " << e.what() << endl;
                }
            }
        }

        if (!matched)
        {
            vector<string> nullRow = row1;
            for (size_t i = 0; i < table2Columns.size(); ++i)
            {
                if (i != static_cast<size_t>(table2JoinIdx))
                {
                    nullRow.push_back("NULL");
                }
            }
            try
            {
                joinedTable.insert(nullRow);
            }
            catch (const exception &e)
            {
                cerr << "Error inserting unmatched row: " << e.what() << endl;
            }
        }
    }

    table1File.close();
    table2File.close();

    cout << "Left Join Result:" << endl;
    joinedTable.displayTable(vector<string>());
    drop(db, "temp_joined_table");
}

void fullJoin(Database &db, Table &table1, Table &table2, const vector<string> &joinColumns)
{
    if (joinColumns.size() < 2)
    {
        cerr << "Error: Need exactly two columns for join condition (table1.col and table2.col)" << endl;
        return;
    }

    if (db.tableExists("temp_joined_table"))
    {
        cout << "Cleaning up existing temp_joined_table..." << endl;
        truncate(db, "temp_joined_table");
    }

    vector<string> table1Columns, table2Columns;
    vector<string> table1Types, table2Types;

    for (const auto &pair : table1.columns)
    {
        table1Columns.push_back(pair.first);
        table1Types.push_back(datatypeName[table1.columns[pair.first].second]);
    }

    for (const auto &pair : table2.columns)
    {
        table2Columns.push_back(pair.first);
        table2Types.push_back(datatypeName[table2.columns[pair.first].second]);
    }

    string table1JoinCol = joinColumns[0];
    string table2JoinCol = joinColumns[1];

    if (table1.columns.find(table1JoinCol) == table1.columns.end())
    {
        cerr << "Error: Join column '" << table1JoinCol << "' not found in table " << table1.getName() << endl;
        return;
    }

    if (table2.columns.find(table2JoinCol) == table2.columns.end())
    {
        cerr << "Error: Join column '" << table2JoinCol << "' not found in table " << table2.getName() << endl;
        return;
    }

    int table1JoinIdx = table1.columns[table1JoinCol].first;
    int table2JoinIdx = table2.columns[table2JoinCol].first;

    vector<string> joinedColumns = table1Columns;
    vector<string> joinedTypes = table1Types;

    for (size_t i = 0; i < table2Columns.size(); ++i)
    {
        if (table2Columns[i] != table2JoinCol)
        {
            joinedColumns.push_back(table2Columns[i]);
            joinedTypes.push_back(table2Types[i]);
        }
    }

    Table joinedTable(db, "temp_joined_table", joinedColumns, joinedTypes);
    db.addTable("temp_joined_table");

    ifstream table1File("./Databases/" + db.getName() + "/" + table1.getName() + "/data.csv");
    ifstream table2File("./Databases/" + db.getName() + "/" + table2.getName() + "/data.csv");

    if (!table1File.is_open() || !table2File.is_open())
    {
        cerr << "Error: Unable to open table data files." << endl;
        return;
    }

    vector<vector<string>> table2Rows;
    string row2Line;

    // Read table2 into memory
    while (getline(table2File, row2Line))
    {
        stringstream ss(row2Line);
        string cell;
        vector<string> row;
        while (getline(ss, cell, ','))
            row.push_back(cell);
        table2Rows.push_back(row);
    }

    vector<bool> matchedTable2(table2Rows.size(), false);

    string row1Line;
    while (getline(table1File, row1Line))
    {
        stringstream ss(row1Line);
        string cell;
        vector<string> row1;
        while (getline(ss, cell, ','))
            row1.push_back(cell);

        bool matched = false;

        for (size_t i = 0; i < table2Rows.size(); ++i)
        {
            const auto &row2 = table2Rows[i];
            if (row1[table1JoinIdx] == row2[table2JoinIdx])
            {
                matched = true;
                matchedTable2[i] = true;
                vector<string> combinedRow = row1;
                for (size_t j = 0; j < row2.size(); ++j)
                {
                    if (j != static_cast<size_t>(table2JoinIdx))
                        combinedRow.push_back(row2[j]);
                }
                joinedTable.insert(combinedRow);
            }
        }

        if (!matched)
        {
            vector<string> nullRow = row1;
            for (size_t j = 0; j < table2Columns.size(); ++j)
            {
                if (j != static_cast<size_t>(table2JoinIdx))
                    nullRow.push_back("NULL");
            }
            joinedTable.insert(nullRow);
        }
    }

    // Add remaining unmatched rows from table2
    for (size_t i = 0; i < table2Rows.size(); ++i)
    {
        if (!matchedTable2[i])
        {
            vector<string> nullRow;
            for (size_t j = 0; j < table1Columns.size(); ++j)
            {
                if (j != static_cast<size_t>(table1JoinIdx))
                    nullRow.push_back("NULL");
                else
                    nullRow.push_back(table2Rows[i][table2JoinIdx]);
            }

            for (size_t j = 0; j < table2Rows[i].size(); ++j)
            {
                if (j != static_cast<size_t>(table2JoinIdx))
                    nullRow.push_back(table2Rows[i][j]);
            }

            joinedTable.insert(nullRow);
        }
    }

    table1File.close();
    table2File.close();

    cout << "Full Join Result:" << endl;
    joinedTable.displayTable(vector<string>());
    drop(db, "temp_joined_table");
}

// void crossJoin(Database& db,  Table&  table1,  Table&  table2) {

//     // Paths to table data
//     string table1Path = "./Databases/" + db.getName() + "/" +  table1 + "/data.csv";
//     string table2Path = "./Databases/" + db.getName() + "/" +  table2 + "/data.csv";

//     // Load table data
//     vector<string> table1Columns, table2Columns;
//     vector<vector<string>> table1Data = loadTableDataFromFile(table1Path, table1Columns);
//     vector<vector<string>> table2Data = loadTableDataFromFile(table2Path, table2Columns);

//     if (table1Data.empty() || table2Data.empty()) {
//         cerr << "Failed to load table data!" << endl;
//         return;
//     }

//     // Create a temporary file for the result
//     string tempFilePath = "./Databases/" + db.getName() + "/temp_cross_join.csv";
//     ofstream tempFile(tempFilePath);

//     if (!tempFile.is_open()) {
//         cerr << "Failed to create temporary file for cross join!" << endl;
//         return;
//     }

//     // Write headers
//     for (const auto& col : table1Columns) tempFile << col << ",";
//     for (const auto& col : table2Columns) tempFile << col << ",";
//     tempFile.seekp(-1, ios::cur); // Remove trailing comma
//     tempFile << endl;

//     // Perform the cross join
//     for (const auto& row1 : table1Data) {
//         for (const auto& row2 : table2Data) {
//             for (const auto& val : row1) tempFile << val << ",";
//             for (const auto& val : row2) tempFile << val << ",";
//             tempFile.seekp(-1, ios::cur); // Remove trailing comma
//             tempFile << endl;
//         }
//     }

//     tempFile.close();

//     // Output the result
//     ifstream resultFile(tempFilePath);
//     if (resultFile.is_open()) {
//         cout << "Cross Join Result:" << endl;
//         string line;
//         while (getline(resultFile, line)) {
//             cout << line << endl;
//         }
//         resultFile.close();
//     }

//     // Delete the temporary file
//     filesystem::remove(tempFilePath);
// }
// void selfJoin(Database& db,  Table& tableName, const vector<string>& joinColumns) {
//     // Check if the table exists
//     if (!db.tableExists(tableName)) {
//         cerr << "Table does not exist!" << endl;
//         return;
//     }

//     // Paths to table data
//     string tablePath = "./Databases/" + db.getName() + "/" + tableName + "/data.csv";

//     // Load table data
//     vector<string> tableColumns;
//     vector<vector<string>> tableData = loadTableDataFromFile(tablePath, tableColumns);

//     if (tableData.empty()) {
//         cerr << "Failed to load table data!" << endl;
//         return;
//     }

//     // Create a temporary file for the result
//     string tempFilePath = "./Databases/" + db.getName() + "/temp_self_join.csv";
//     ofstream tempFile(tempFilePath);

//     if (!tempFile.is_open()) {
//         cerr << "Failed to create temporary file for self join!" << endl;
//         return;
//     }

//     // Write headers
//     for (const auto& col : tableColumns) tempFile << tableName + "_1." + col << ",";
//     for (const auto& col : tableColumns) tempFile << tableName + "_2." + col << ",";
//     tempFile.seekp(-1, ios::cur); // Remove trailing comma
//     tempFile << endl;

//     // Perform the self join
//     for (const auto& row1 : tableData) {
//         for (const auto& row2 : tableData) {
//             bool match = true;
//             for (const auto& joinCol : joinColumns) {
//                 if (row1[tableColumns.size()] != row2[tableColumns.size()]) {
//                     match = false;
//                     break;
//                 }
//             }
//             if (match) {
//                 for (const auto& val : row1) tempFile << val << ",";
//                 for (const auto& val : row2) tempFile << val << ",";
//                 tempFile.seekp(-1, ios::cur); // Remove trailing comma
//                 tempFile << endl;
//             }
//         }
//     }

//     tempFile.close();

//     // Output the result
//     ifstream resultFile(tempFilePath);
//     if (resultFile.is_open()) {
//         cout << "Self Join Result:" << endl;
//         string line;
//         while (getline(resultFile, line)) {
//             cout << line << endl;
//         }
//         resultFile.close();
//     }

//     // Delete the temporary file
//     filesystem::remove(tempFilePath);
// }

// void displayMenu() {
//     cout << "Join Operations:" << endl;
//     cout << "1. Inner Join" << endl;
//     cout << "2. Right Join" << endl;
//     cout << "3. Left Join" << endl;
//     cout << "4. Full Join" << endl;
//     cout << "5. Cross Join" << endl;
//     cout << "6. Self Join" << endl;
//     cout << "7. Exit" << endl;
// }

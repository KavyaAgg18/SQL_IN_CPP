#include <iostream>
#include <vector>
#include "join.h"
#include <unordered_map>
#include <string>
#include <filesystem>
#include "table.h"
#include "database.h"
#include "globals.h"

using namespace std;

// INNER JOIN
// O(n + m) via hash index on the smaller table's join column, replacing the previous O(n*m) nested scan.
void innerJoin(Database &db, Table &table1, Table &table2, const vector<string> &joinColumns,
               const vector<string> &projectedColumns)
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

    // ── schema setup (unchanged) ──────────────────────────────────────────────
    vector<string> table1Columns, table2Columns;
    vector<string> table1Types,   table2Types;

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
    vector<string> joinedTypes   = table1Types;
    for (size_t i = 0; i < table2Columns.size(); ++i)
        if (table2Columns[i] != table2JoinCol)
        {
            joinedColumns.push_back(table2Columns[i]);
            joinedTypes.push_back(table2Types[i]);
        }

    Table joinedTable(db, "temp_joined_table", joinedColumns, joinedTypes);
    db.addTable("temp_joined_table");

    // ── hash-based matching ───────────────────────────────────────────────────
    table1.loadRows();
    table2.loadRows();

    // Index the smaller table for O(1) probing.
    bool indexTable1 = (table1.getRows().size() <= table2.getRows().size());
    auto index = indexTable1 ? table1.buildIndex(table1JoinCol)
                             : table2.buildIndex(table2JoinCol);

    const vector<Row> &scanRows  = indexTable1 ? table2.getRows() : table1.getRows();
    const vector<Row> &indexRows = indexTable1 ? table1.getRows() : table2.getRows();
    int scanJoinIdx = indexTable1 ? table2JoinIdx : table1JoinIdx;

    for (const auto &scanRow : scanRows)
    {
        if (scanJoinIdx >= static_cast<int>(scanRow.cells.size())) continue;
        const Value &key = scanRow.cells[scanJoinIdx];
        auto it = index.find(key);
        if (it == index.end()) continue;

        for (int idx : it->second)
        {
            const Row &t1Row = indexTable1 ? indexRows[idx] : scanRow;
            const Row &t2Row = indexTable1 ? scanRow        : indexRows[idx];

            vector<string> combinedRow;
            for (const auto &cell : t1Row.cells)
                combinedRow.push_back(valueToString(cell));
            for (size_t i = 0; i < t2Row.cells.size(); ++i)
                if (i != static_cast<size_t>(table2JoinIdx))
                    combinedRow.push_back(valueToString(t2Row.cells[i]));

            try { joinedTable.insert(combinedRow); }
            catch (const exception &e) { cerr << "Error inserting row: " << e.what() << endl; }
        }
    }

    cout << "Inner Join Result:" << endl;
    joinedTable.displayTable(projectedColumns);
    drop(db, "temp_joined_table");
}

// RIGHT JOIN
// O(n + m) via hash index on table1's join column; scans table2 as the driving side.
void rightJoin(Database &db, Table &table1, Table &table2, const vector<string> &joinColumns,
               const vector<string> &projectedColumns)
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

    // ── schema setup (unchanged) ──────────────────────────────────────────────
    vector<string> table1Columns, table2Columns;
    vector<string> table1Types,   table2Types;

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

    int table2JoinIdx = table2.columns[table2JoinCol].first;

    vector<string> joinedColumns = table1Columns;
    vector<string> joinedTypes   = table1Types;
    for (size_t i = 0; i < table2Columns.size(); ++i)
        if (table2Columns[i] != table2JoinCol)
        {
            joinedColumns.push_back(table2Columns[i]);
            joinedTypes.push_back(table2Types[i]);
        }

    Table joinedTable(db, "temp_joined_table", joinedColumns, joinedTypes);
    db.addTable("temp_joined_table");

    // ── hash-based matching ───────────────────────────────────────────────────
    table1.loadRows();
    table2.loadRows();

    // Always index table1 and drive over table2 so every table2 row is visited
    // (required to emit unmatched table2 rows correctly for RIGHT JOIN).
    auto index = table1.buildIndex(table1JoinCol);

    for (const auto &t2Row : table2.getRows())
    {
        if (table2JoinIdx >= static_cast<int>(t2Row.cells.size())) continue;
        const Value &key = t2Row.cells[table2JoinIdx];
        auto it = index.find(key);

        if (it != index.end())
        {
            for (int idx : it->second)
            {
                const Row &t1Row = table1.getRows()[idx];
                vector<string> combinedRow;
                for (const auto &cell : t1Row.cells)
                    combinedRow.push_back(valueToString(cell));
                for (size_t i = 0; i < t2Row.cells.size(); ++i)
                    if (i != static_cast<size_t>(table2JoinIdx))
                        combinedRow.push_back(valueToString(t2Row.cells[i]));
                try { joinedTable.insert(combinedRow); }
                catch (const exception &e) { cerr << "Error inserting row: " << e.what() << endl; }
            }
        }
        else
        {
            // Unmatched table2 row: fill table1 side with NULLs.
            vector<string> nullRow(table1Columns.size(), "NULL");
            for (size_t i = 0; i < t2Row.cells.size(); ++i)
                if (i != static_cast<size_t>(table2JoinIdx))
                    nullRow.push_back(valueToString(t2Row.cells[i]));
            try { joinedTable.insert(nullRow); }
            catch (const exception &e) { cerr << "Error inserting unmatched row: " << e.what() << endl; }
        }
    }

    cout << "Right Join Result:" << endl;
    joinedTable.displayTable(projectedColumns);
    drop(db, "temp_joined_table");
}

// LEFT JOIN
// O(n + m) via hash index on table2's join column; scans table1 as the driving side.
void leftJoin(Database &db, Table &table1, Table &table2, const vector<string> &joinColumns,
              const vector<string> &projectedColumns)
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

    // ── schema setup (unchanged) ──────────────────────────────────────────────
    vector<string> table1Columns, table2Columns;
    vector<string> table1Types,   table2Types;

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
    vector<string> joinedTypes   = table1Types;
    for (size_t i = 0; i < table2Columns.size(); ++i)
        if (table2Columns[i] != table2JoinCol)
        {
            joinedColumns.push_back(table2Columns[i]);
            joinedTypes.push_back(table2Types[i]);
        }

    Table joinedTable(db, "temp_joined_table", joinedColumns, joinedTypes);
    db.addTable("temp_joined_table");

    // ── hash-based matching ───────────────────────────────────────────────────
    table1.loadRows();
    table2.loadRows();

    // Always index table2 and drive over table1 so every table1 row is visited
    // (required to emit unmatched table1 rows correctly for LEFT JOIN).
    auto index = table2.buildIndex(table2JoinCol);

    for (const auto &t1Row : table1.getRows())
    {
        if (table1JoinIdx >= static_cast<int>(t1Row.cells.size())) continue;
        const Value &key = t1Row.cells[table1JoinIdx];
        auto it = index.find(key);

        if (it != index.end())
        {
            for (int idx : it->second)
            {
                const Row &t2Row = table2.getRows()[idx];
                vector<string> combinedRow;
                for (const auto &cell : t1Row.cells)
                    combinedRow.push_back(valueToString(cell));
                for (size_t i = 0; i < t2Row.cells.size(); ++i)
                    if (i != static_cast<size_t>(table2JoinIdx))
                        combinedRow.push_back(valueToString(t2Row.cells[i]));
                try { joinedTable.insert(combinedRow); }
                catch (const exception &e) { cerr << "Error inserting row: " << e.what() << endl; }
            }
        }
        else
        {
            // Unmatched table1 row: fill table2 side with NULLs.
            vector<string> nullRow;
            for (const auto &cell : t1Row.cells)
                nullRow.push_back(valueToString(cell));
            for (size_t i = 0; i < table2Columns.size(); ++i)
                if (i != static_cast<size_t>(table2JoinIdx))
                    nullRow.push_back("NULL");
            try { joinedTable.insert(nullRow); }
            catch (const exception &e) { cerr << "Error inserting unmatched row: " << e.what() << endl; }
        }
    }

    cout << "Left Join Result:" << endl;
    joinedTable.displayTable(projectedColumns);
    drop(db, "temp_joined_table");
}

// FULL JOIN
// O(n + m) via hash index on table2's join column; unmatched rows from both sides are emitted.
void fullJoin(Database &db, Table &table1, Table &table2, const vector<string> &joinColumns,
              const vector<string> &projectedColumns)
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

    // ── schema setup (unchanged) ──────────────────────────────────────────────
    vector<string> table1Columns, table2Columns;
    vector<string> table1Types,   table2Types;

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
    vector<string> joinedTypes   = table1Types;
    for (size_t i = 0; i < table2Columns.size(); ++i)
        if (table2Columns[i] != table2JoinCol)
        {
            joinedColumns.push_back(table2Columns[i]);
            joinedTypes.push_back(table2Types[i]);
        }

    Table joinedTable(db, "temp_joined_table", joinedColumns, joinedTypes);
    db.addTable("temp_joined_table");

    // ── hash-based matching ───────────────────────────────────────────────────
    table1.loadRows();
    table2.loadRows();

    // Index table2; track which table2 rows were matched for the outer-join pass.
    auto index = table2.buildIndex(table2JoinCol);
    vector<bool> matchedTable2(table2.getRows().size(), false);

    for (const auto &t1Row : table1.getRows())
    {
        if (table1JoinIdx >= static_cast<int>(t1Row.cells.size())) continue;
        const Value &key = t1Row.cells[table1JoinIdx];
        auto it = index.find(key);

        if (it != index.end())
        {
            for (int idx : it->second)
            {
                matchedTable2[idx] = true;
                const Row &t2Row = table2.getRows()[idx];
                vector<string> combinedRow;
                for (const auto &cell : t1Row.cells)
                    combinedRow.push_back(valueToString(cell));
                for (size_t i = 0; i < t2Row.cells.size(); ++i)
                    if (i != static_cast<size_t>(table2JoinIdx))
                        combinedRow.push_back(valueToString(t2Row.cells[i]));
                joinedTable.insert(combinedRow);
            }
        }
        else
        {
            // Unmatched table1 row: table2 side is NULLed.
            vector<string> nullRow;
            for (const auto &cell : t1Row.cells)
                nullRow.push_back(valueToString(cell));
            for (size_t j = 0; j < table2Columns.size(); ++j)
                if (j != static_cast<size_t>(table2JoinIdx))
                    nullRow.push_back("NULL");
            joinedTable.insert(nullRow);
        }
    }

    // Emit unmatched table2 rows: table1 side is NULLed, except the join column
    // position which carries the table2 join value (mirrors original behaviour).
    for (size_t i = 0; i < table2.getRows().size(); ++i)
    {
        if (matchedTable2[i]) continue;
        const Row &t2Row = table2.getRows()[i];
        vector<string> nullRow;
        for (size_t j = 0; j < table1Columns.size(); ++j)
        {
            if (j != static_cast<size_t>(table1JoinIdx))
                nullRow.push_back("NULL");
            else
                nullRow.push_back(valueToString(t2Row.cells[table2JoinIdx]));
        }
        for (size_t j = 0; j < t2Row.cells.size(); ++j)
            if (j != static_cast<size_t>(table2JoinIdx))
                nullRow.push_back(valueToString(t2Row.cells[j]));
        joinedTable.insert(nullRow);
    }

    cout << "Full Join Result:" << endl;
    joinedTable.displayTable(projectedColumns);
    drop(db, "temp_joined_table");
}

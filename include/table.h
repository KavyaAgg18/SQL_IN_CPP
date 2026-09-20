#ifndef TABLE_H
#define TABLE_H

#include "database.h"
#include "value.h"
#include <unordered_map>
#include <vector>
#include <string>

using namespace std;

class Table
{
private:
    Database &db;
    string tableName; // Table name

    // Converts a flat CSV-split field vector into a typed Row using this table's
    // column schema. Used by loadRows() (Phase 2) and buildIndex() (Phase 3).
    Row parseRow(const vector<string> &rawFields) const;

    vector<Row> rows;       // in-memory row cache populated by loadRows()
    bool rowsLoaded = false; // guards against redundant disk reads within a session

public:
    unordered_map<string, pair<int, int>> columns; // column name -> (index, datatype ID)
    Table();
    Table(Database &db, string tableName = "", const vector<string> &columnName = {}, const vector<string> &type = {});
    string getName() const { return tableName; }

    // Loads data.csv into the in-memory rows cache (no-op if already loaded).
    // Called by displayTable() and, starting Phase 3, by buildIndex().
    void loadRows();

    void insert(const vector<string> &rowData);
    void insertWithColumns(const vector<string> &columnNames, const vector<string> &rowData);
    void displayTable(const vector<string> &columnNames);
};

void create(Database &db, const string &tableName, const vector<string> &columns, const vector<string> &types);
Table selectTable(Database &db, const string &tableName);

void rename(Database &db, const string &oldName, const string &newName);
void drop(Database &db, const string &tableName);
void truncate(Database &db, const string &tableName);

#endif // TABLE_H

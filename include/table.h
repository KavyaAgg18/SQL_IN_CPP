#ifndef TABLE_H
#define TABLE_H

#include "database.h"
#include <unordered_map>
#include <vector>
#include <string>

using namespace std;

class Table
{
private:
    Database &db;
    string tableName; // Table name

public:
    unordered_map<string, pair<int, int>> columns; // column name -> (index, datatype ID)
    Table();
    Table(Database &db, string tableName = "", const vector<string> &columnName = {}, const vector<string> &type = {});
    string getName() const { return tableName; }

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

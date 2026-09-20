#ifndef JOIN_H
#define JOIN_H

#include <string>
#include <vector>
#include "database.h"
#include "table.h"

// Function declarations
vector<vector<string>> loadTableDataFromFile(const string &filePath, vector<string> &columns);

void innerJoin(Database &db, Table &table1, Table &table2,
               const vector<string> &joinColumns,
               const vector<string> &projectedColumns);

void rightJoin(Database &db, Table &table1, Table &table2,
               const vector<string> &joinColumns,
               const vector<string> &projectedColumns);

void leftJoin(Database &db, Table &table1, Table &table2,
              const vector<string> &joinColumns,
              const vector<string> &projectedColumns);

void fullJoin(Database &db, Table &table1, Table &table2,
              const vector<string> &joinColumns,
              const vector<string> &projectedColumns);

// void crossJoin(Database &db, Table &table1, Table &table2);

// void selfJoin(Database &db, Table &tableName, const vector<string> &joinColumns);

#endif // JOIN_H
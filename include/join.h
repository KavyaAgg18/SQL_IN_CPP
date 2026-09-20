#ifndef JOIN_H
#define JOIN_H

#include <string>
#include <vector>
#include "database.h"
#include "table.h"

enum class JoinType { INNER, LEFT, RIGHT, FULL };

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


#endif // JOIN_H
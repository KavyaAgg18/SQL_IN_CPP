// sqlparser.h
#ifndef SQLPARSER_H
#define SQLPARSER_H

#include "database.h"

class SQLParser {
public:
    static void executeQuery(Database& db, const std::string& query);
};

#endif // SQLPARSER_H

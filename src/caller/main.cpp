#include "database.h"
#include "table.h"
#include "sqlparser.h"
#include <iostream>
#include <string>
#include <sstream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <cstdlib>
#endif

using namespace std;

#ifdef __EMSCRIPTEN__
EM_ASYNC_JS(char*, webReadLineRaw, (), {
    if (typeof window.awaitInputLine !== "function") {
        return 0;
    }
    const value = await window.awaitInputLine();
    const text = String(value || "");
    const len = lengthBytesUTF8(text) + 1;
    const ptr = _malloc(len);
    stringToUTF8(text, ptr, len);
    return ptr;
});
#endif

static string readLinePrompt(const string& prompt) {
    cout << prompt;
    cout.flush();
#ifdef __EMSCRIPTEN__
    char* raw = webReadLineRaw();
    if (raw == nullptr) {
        return "";
    }
    string line(raw);
    free(raw);
    return line;
#else
    string line;
    getline(cin, line);
    return line;
#endif
}

static int readIntPrompt(const string& prompt) {
    while (true) {
        string line = readLinePrompt(prompt);
        stringstream ss(line);
        int value;
        char extra;
        if ((ss >> value) && !(ss >> extra)) {
            return value;
        }
        cerr << "Invalid choice. Try again.\n";
    }
}

void sqlQueryMenu(Database& db) {
    while (true) {
        cout << "\nEnter SQL Query (or type EXIT to go back):\n";
        string query = readLinePrompt("SQL> ");
        if (query == "EXIT") break;
        SQLParser::executeQuery(db, query);
    }
}

void mainMenu() {
    string dbName;
    Database db;

    while (true) {
        cout << "\nMain Menu:\n";
        cout << "1. Create Database\n";
        cout << "2. Select Database\n";
        cout << "3. Show Databases\n";
        cout << "4. Exit\n";
        int choice = readIntPrompt("Enter your choice: ");

        switch (choice) {
        case 1:
            dbName = readLinePrompt("Enter database name: ");
            db = createDatabase(dbName);
            if (db.isValid()) {
                cout << "Database created successfully." << endl;
            } else {
                cerr << "Failed to create database." << endl;
            }
            break;

        case 2:
            dbName = readLinePrompt("Enter database name: ");
            db = selectDatabase(dbName);
            if (db.isValid()) {
                cout << "Database '" << dbName << "' selected successfully." << endl;
                sqlQueryMenu(db);
            } else {
                cerr << "Failed to select database '" << dbName << "'." << endl;
            }
            break;

        case 3:
            displayDatabases();
            break;

        case 4:
            cout << "Exiting...\n";
            return;

        default:
            cerr << "Invalid choice. Try again.\n";
        }
    }
}

int main() {
    cout.setf(ios::unitbuf);
    cerr.setf(ios::unitbuf);

    initializeDatabaseSystem();
    cout << "Database system initialized." << endl;
    mainMenu();
    return 0;
}

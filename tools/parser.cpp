#include <iostream>
#include <vector>
#include <unordered_set>
#include <cctype>
#include <regex>

using namespace std;

struct Token {
    string type;
    string value;
};

// Function to tokenize SQL queries
vector<Token> tokenize(const string &query, bool &isValid) {
    unordered_set<string> keywords = {
        "SELECT", "FROM", "WHERE", "INSERT", "UPDATE", "DELETE", "AND", "OR", "NOT", 
        "INTO", "VALUES", "SET", "IN", "NULL", "LIKE", "BETWEEN", "JOIN", "ON", 
        "ORDER", "GROUP", "BY", "HAVING", "AS", "DISTINCT", "CASE", "WHEN", "THEN", 
        "ELSE", "END", "LIMIT", "OFFSET", "CREATE", "TABLE", "DATABASE", "PRIMARY", 
        "KEY", "FOREIGN", "REFERENCES", "VARCHAR", "INT", "FLOAT", "CHAR", "TEXT"
    };
    unordered_set<string> operators = {">", "<", "=", ">=", "<=", "!=", "==", "*", "+", "-", "/"};

    vector<Token> tokens;
    regex word_regex(R"('.*?'|\b[a-zA-Z_][a-zA-Z0-9_]*\b|\d+(\.\d+)?|>=|<=|!=|==|[<>*=(),;])", regex::icase);
    
    auto words_begin = sregex_iterator(query.begin(), query.end(), word_regex);
    auto words_end = sregex_iterator();

    isValid = true;  // Assume query is valid initially

    for (auto i = words_begin; i != words_end; ++i) {
        string word = i->str();
        string upper_word = word;
        transform(upper_word.begin(), upper_word.end(), upper_word.begin(), ::toupper);

        if (keywords.count(upper_word)) {
            tokens.push_back({"KEYWORD", word});
        } else if (operators.count(word)) {
            tokens.push_back({"OPERATOR", word});
        } else if (isdigit(word[0]) || (word[0] == '-' && isdigit(word[1]))) {
            tokens.push_back({"LITERAL", word});
        } else if (word[0] == '\'') {
            tokens.push_back({"LITERAL", word});
        } else if (word == "," || word == ";" || word == "(" || word == ")") {
            tokens.push_back({"SYMBOL", word});
        } else if (regex_match(word, regex(R"([a-zA-Z_][a-zA-Z0-9_]*)"))) {
            tokens.push_back({"IDENTIFIER", word});
        } else {
            cout << "\n❌ ERROR: Invalid token found -> " << word << endl;
            isValid = false;
            return {}; // Return empty tokens on error
        }
    }
    return tokens;
}

int main() {
    string query;
    bool isValid;

    do {
        cout << "\nEnter an SQL query: ";
        getline(cin, query);

        vector<Token> tokens = tokenize(query, isValid);

        if (isValid) {
            cout << "\n✅ Tokenized Output:\n";
            for (const auto &token : tokens) {
                cout << "Type: " << token.type << ", Value: " << token.value << endl;
            }
        } else {
            cout << "\n⚠️ Please enter a valid SQL query again.\n";
        }

    } while (!isValid);

    return 0;
}

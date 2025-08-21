#include <iostream>
#include <vector>
#include <string>
#include <unordered_set>
#include <map>
#include <cctype>

using namespace std;

// Token Types
enum TokenType { KEYWORD, IDENTIFIER, OPERATOR, NUMBER, UNKNOWN };

// Token Structure
struct Token {
    TokenType type;
    string value;
};

// Symbol Table, Intermediate Code, and Temp Counter
unordered_set<string> symbolTable;
vector<string> intermediateCode;
vector<string> optimizedCode;
int tempCount = 1;

// Optimization Maps
map<string, string> valueMap;  // For copy propagation
map<string, string> tempMap;   // To map temps to values

// Keywords and Operators
unordered_set<string> keywords = {"def", "let", "print", "if", "end"};
unordered_set<string> operators = {"+", "-", "*", "/", "^", "=", "(", ")", ","};

// Lexer Function
vector<Token> tokenize(const string& code) {
    vector<Token> tokens;
    string buffer;

    for (size_t i = 0; i < code.size(); ++i) {
        char c = code[i];

        if (isspace(c)) {
            if (!buffer.empty()) {
                if (keywords.count(buffer)) {
                    tokens.push_back({KEYWORD, buffer});
                } else if (isdigit(buffer[0])) {
                    tokens.push_back({NUMBER, buffer});
                } else {
                    tokens.push_back({IDENTIFIER, buffer});
                }
                buffer.clear();  
            }
        } else if (operators.count(string(1, c))) {
            if (!buffer.empty()) {
                if (keywords.count(buffer)) {
                    tokens.push_back({KEYWORD, buffer});
                } else if (isdigit(buffer[0])) {
                    tokens.push_back({NUMBER, buffer});
                } else {
                    tokens.push_back({IDENTIFIER, buffer});
                }
                buffer.clear();
            }
            tokens.push_back({OPERATOR, string(1, c)});
        } else {
            buffer += c;
        }
    }

    if (!buffer.empty()) {
        if (keywords.count(buffer)) {
            tokens.push_back({KEYWORD, buffer});
        } else if (isdigit(buffer[0])) {
            tokens.push_back({NUMBER, buffer});
        } else {
            tokens.push_back({IDENTIFIER, buffer});
        }
    }

    return tokens;
}

// Generate Temporary Variable
string getTempVar() {
    return "t" + to_string(tempCount++);
}

// Intermediate Code Generation with Optimization
string generateExpressionCode(const vector<Token>& tokens, size_t& index) {
    string result;

    if (index < tokens.size() && tokens[index].value == "(") {
        ++index;  // Consume '('
        result = generateExpressionCode(tokens, index);
        ++index;  // Consume ')'
    } else if (tokens[index].type == IDENTIFIER || tokens[index].type == NUMBER) {
        result = tokens[index].value;
        ++index;
    }

    while (index < tokens.size() && tokens[index].type == OPERATOR) {
        string op = tokens[index].value;
        ++index;

        string right = generateExpressionCode(tokens, index);

        // Constant Folding Optimization
        if (isdigit(result[0]) && isdigit(right[0])) {
            int leftVal = stoi(result);
            int rightVal = stoi(right);

            int res = 0;
            if (op == "+") res = leftVal + rightVal;
            else if (op == "-") res = leftVal - rightVal;
            else if (op == "*") res = leftVal * rightVal;
            else if (op == "/") res = leftVal / rightVal;

            result = to_string(res);
        } else {
            string temp = getTempVar();
            intermediateCode.push_back(temp + " = " + result + " " + op + " " + right);

            // Copy Propagation
            tempMap[temp] = result + " " + op + " " + right;
            result = temp;
        }
    }

    return result;
}

// Syntax and Semantic Analyzer with Intermediate Code Generation
bool parse(const vector<Token>& tokens) {
    size_t index = 0;

    while (index < tokens.size()) {
        if (tokens[index].type == KEYWORD) {
            string keyword = tokens[index].value;
            ++index;

            if (keyword == "let") {
                if (index < tokens.size() && tokens[index].type == IDENTIFIER) {
                    string varName = tokens[index].value;
                    symbolTable.insert(varName);
                    ++index;

                    if (index < tokens.size() && tokens[index].type == OPERATOR && tokens[index].value == "=") {
                        ++index;

                        string result = generateExpressionCode(tokens, index);
                        intermediateCode.push_back(varName + " = " + result);

                        // Store value for copy propagation
                        valueMap[varName] = result;
                    } else {
                        cout << "Syntax Error: Missing '=' after variable declaration." << endl;
                        return false;
                    }
                } else {
                    cout << "Syntax Error: Missing variable name in declaration." << endl;
                    return false;
                }

            } else if (keyword == "print") {
                if (index < tokens.size() && tokens[index].type == IDENTIFIER) {
                    string varName = tokens[index].value;

                    if (symbolTable.find(varName) == symbolTable.end()) {
                        cout << "Semantic Error: Variable '" << varName << "' used before declaration in print." << endl;
                        return false;
                    }

                    intermediateCode.push_back("print " + varName);
                    ++index;

                } else {
                    cout << "Syntax Error: Invalid print statement." << endl;
                    return false;
                }

            } else if (keyword == "end") {
                continue;
            }

        } else {
            cout << "Syntax Error at token: " << tokens[index].value << endl;
            return false;
        }
    }

    return true;
}

// Optimization Function
void optimizeCode() {
    map<string, string> varMap;

    for (const auto& line : intermediateCode) {
        size_t pos = line.find(" = ");
        if (pos != string::npos) {
            string lhs = line.substr(0, pos);
            string rhs = line.substr(pos + 3);

            if (varMap.count(rhs)) {
                rhs = varMap[rhs];  
            }

            varMap[lhs] = rhs;

            if (lhs[0] == 't' && isdigit(lhs[1])) {  
                continue;  
            }

            optimizedCode.push_back(lhs + " = " + rhs);
        } else {
            optimizedCode.push_back(line);
        }
    }
}

// Assembly Code Generation
void generateAssembly() {
    cout << "\nGenerated Assembly Code:\n";
    
    for (const auto& line : optimizedCode) {
        size_t pos = line.find(" = ");
        if (pos != string::npos) {
            string lhs = line.substr(0, pos);
            string rhs = line.substr(pos + 3);

            if (rhs.find(" + ") != string::npos) {
                size_t opPos = rhs.find(" + ");
                string left = rhs.substr(0, opPos);
                string right = rhs.substr(opPos + 3);

                cout << "mov r0, " << left << endl;
                cout << "mov r1, " << right << endl;
                cout << "add r0, r1" << endl;
                cout << "str r0, " << lhs << endl;

            } else {
                cout << "mov " << lhs << ", " << rhs << endl;
            }
        } else if (line.find("print") != string::npos) {
            string var = line.substr(6);
            cout << "print " << var << endl;
        }
    }
}

// Main Execution
int main() {
    string code = R"(
        let x = 10
        let y = 20
        let z = x + y
        print z
    )";

    cout << "\nPerforming Lexical Analysis..." << endl;
    vector<Token> tokens = tokenize(code);

    cout << "\nTokens:" << endl;
    for (const auto& token : tokens) {
        cout << "Type: " << (token.type == KEYWORD ? "KEYWORD" :
                             token.type == IDENTIFIER ? "IDENTIFIER" :
                             token.type == OPERATOR ? "OPERATOR" :
                             token.type == NUMBER ? "NUMBER" : "UNKNOWN")
             << ", Value: " << token.value << endl;
    }

    cout << "\nPerforming Syntax and Semantic Analysis..." << endl;
    if (parse(tokens)) {
        cout << "Syntax and Semantic Analysis Passed: No Errors!" << endl;

        optimizeCode();

        cout << "\nOptimized Intermediate Code:\n";
        for (const auto& line : optimizedCode) {
            cout << line << endl;
        }

        generateAssembly();
    }

    return 0;
}

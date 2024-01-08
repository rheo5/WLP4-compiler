#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <map>
#include "dfa.h"

using namespace std;

//// These helper functions are defined at the bottom of the file:
// Check if a string is a single character.
bool isChar(string s);
// Check if a string represents a character range.
bool isRange(string s);
// Remove leading and trailing whitespace from a string, and
// squish intermediate whitespace sequences down to one space each.
string squish(string s);
// Convert hex digit character to corresponding number.
int hexToNum(char c);
// Convert number to corresponding hex digit character.
char numToHex(int d);
// Replace all escape sequences in a string with corresponding characters.
string escape(string s);
// Convert non-printing characters or spaces in a string into escape sequences.
string unescape(string s);

bool rangedec(string dec);
bool rangehex(string hex);
void printmachinecode(int machinecode);
long long int errordec(string number);
long long int errorhex(string number);

const string STATES = ".STATES";
const string TRANSITIONS = ".TRANSITIONS";
const string INPUT = ".INPUT";

struct Token
{
    string kind;
    string lexeme;
};

struct DFA
{
    vector<string> *states;
    map<string, bool> *accepting;
    map<string, map<char, string>> *nextState; // current state -> character -> next state

    DFA()
    {
        states = new vector<string>;
        accepting = new map<string, bool>;
        nextState = new map<string, map<char, string>>;
    };
    ~DFA()
    {
        delete states;
        delete accepting;
        delete nextState;
    };
};

class mipsscan
{
public:
    vector<Token> simplifiedMaximalMunch(DFA *dfa, istream &inputs)
    {
        string s;
        string lex;
        vector<Token> tokens;
        string currentState = "start";
        string temp;
        string line_str;
        int force = 0;
        bool error = false;

        stringstream buffer;
        buffer << inputs.rdbuf();

        // Append a newline character to the content in the stringstream
        buffer << '\n';

        // Get the content as a string
        string content = buffer.str();

        // Use an istringstream to read lines from the content string
        istringstream contentStream(content);

        while (getline(contentStream, s))
        {
            currentState = "start";
            s.append(" ");
            lex = "";

            if (force != 0)
            {
                Token newToken{"NEWLINE", ""};
                tokens.push_back(newToken);
            }

            force++;

            for (int i = 0; i < s.length(); i++)
            {
                char c = s[i];
                temp = currentState;
                currentState = getNextState(dfa, temp, c);

                if (currentState.empty())
                {
                    if ((*dfa->accepting)[temp] == 0)
                    {
                        cerr << "ERROR\n";
                        error = true;
                    }
                    if (temp == "REGISTER")
                    {
                        try
                        {
                            string number = lex.substr(1);
                            long long num1 = std::stoll(number);
                            if (num1 > 31)
                            {
                                cerr << "ERROR: Invalid register number\n";
                                error = true;
                            }
                        }
                        catch (const std::exception &e)
                        {
                            cerr << "ERROR: Invalid register number\n";
                            error = true;
                        }
                    }
                    if (temp == "DECINT")
                    {
                        try
                        {
                            long long num2 = std::stoll(lex);
                            if (!(num2 >= -2147483648LL && num2 <= 4294967295LL))
                            {
                                cerr << "ERROR: DECINT must be within -2147483648 and 4294967295\n";
                                error = true;
                            }
                        }
                        catch (const std::exception &e)
                        {
                            cerr << "ERROR: DECINT must be within -2147483648 and 4294967295\n";
                            error = true;
                        }
                    }
                    if (temp == "HEXINT")
                    {
                        try
                        {
                            unsigned long long num3 = stoull(lex, NULL, 16);
                            if (!(num3 <= 0xFFFFFFFFLL))
                            {
                                cerr << "ERROR: HEXINT must be below 0xFFFFFFFF\n";
                                error = true;
                            }
                        }
                        catch (const std::exception &e)
                        {
                            cerr << "ERROR: HEXINT must be below 0xFFFFFFFF\n";
                            error = true;
                        }
                    }
                    if (error)
                    {
                        return {};
                    }

                    error = false;

                    if (temp == "ZERO")
                    {
                        temp = "DECINT";
                    }
                    if (temp[0] != '?')
                    {
                        Token newToken{temp, lex};
                        tokens.push_back(newToken);
                    }

                    currentState = "start";
                    lex = "";

                    i--;
                }

                else
                {
                    lex += c;
                }
            }
        }

        return tokens;
    }

    string getNextState(DFA *dfa, string currentState, char c)
    {
        if ((*dfa->nextState)[currentState][c] != "")
        { // Check if there is a next state c;
            return (*dfa->nextState)[currentState][c];
        }
        return "";
    }
};

DFA *createDFA(istream &in)
{
    DFA *dfa = new DFA;
    string s;
    // Skip blank lines at the start of the file
    while (true)
    {
        if (!(getline(in, s)))
        {
            throw runtime_error("ERROR: Expected " + STATES + ", but found end of input.");
        }
        s = squish(s);
        if (s == STATES)
        {
            break;
        }
        if (!s.empty())
        {
            throw runtime_error("ERROR: Expected " + STATES + ", but found: " + s);
        }
    }
    // Print states
    // cout << "States:" << '\n';
    while (true)
    {
        if (!(in >> s))
        {
            throw runtime_error("ERROR: Unexpected end of input while reading state set: " + TRANSITIONS + "not found.");
        }
        if (s == TRANSITIONS)
        {
            break;
        }
        // Process an individual state
        bool accepting = false;
        if (s.back() == '!' && s.length() > 1)
        {
            accepting = true;
            s.pop_back();
        }
        dfa->states->push_back(s);
        (*(dfa->accepting))[s] = accepting;
    }
    // Print transitions
    // cout << "Transitions:" << '\n';
    getline(in, s); // Skip .TRANSITIONS header
    while (true)
    {
        if (!(getline(in, s)))
        {
            // We reached the end of the file
            break;
        }
        s = squish(s);
        if (s == INPUT)
        {
            break;
        }
        // Split the line into parts
        string lineStr = s;
        stringstream line(lineStr);
        vector<string> lineVec;
        while (line >> s)
        {
            lineVec.push_back(s);
        }
        if (lineVec.empty())
        {
            // Skip blank lines
            continue;
        }
        if (lineVec.size() < 3)
        {
            throw runtime_error("ERROR: Incomplete transition line: " + lineStr);
        }
        // Extract state information from the line
        string fromState = lineVec.front();
        string toState = lineVec.back();
        // Extract character and range information from the line
        vector<char> charVec;
        for (int i = 1; i < lineVec.size() - 1; ++i)
        {
            string charOrRange = escape(lineVec[i]);
            if (isChar(charOrRange))
            {
                char c = charOrRange[0];
                if (c < 0 || c > 127)
                {
                    throw runtime_error("ERROR: Invalid (non-ASCII) character in transition line: " + lineStr + "\n" + "Character " + unescape(string(1, c)) + " is outside ASCII range");
                }
                charVec.push_back(c);
            }
            else if (isRange(charOrRange))
            {
                for (char c = charOrRange[0]; charOrRange[0] <= c && c <= charOrRange[2]; ++c)
                {
                    charVec.push_back(c);
                }
            }
            else
            {
                throw runtime_error("ERROR: Expected character or range, but found " + charOrRange + " in transition line: " + lineStr);
            }
        }
        // Print a representation of the transition line
        // cout << fromState << ' ';
        for (char c : charVec)
        {
            (*(dfa->nextState))[fromState][c] = toState;
        }
        // cout << toState << '\n';
    }
    // We ignore .INPUT sections, so we're done
    return dfa;
}

// first pass : syntax checking + symboltable
map<string, int> firstpass(vector<Token> tokens)
{
    map<string, int> symboltable;
    symboltable.insert({"0", 0});

    string ids[] = {"add", "sub", "mult", "multu", "div", "divu", "mfhi", "mflo", "lis", "slt", "sltu", "jr", "jalr", "beq", "bne", "lw", "sw"};
    int line = 0;
    int start = 1;
    int temp = 0;
    int decval = 0;
    long unsigned int hexval = 0;
    int no = 1;

    for (int i = 0; i < tokens.size(); i++)
    {
        temp = 0;
        // check valid id
        // ps : will not run when label at .word label, labeldef label
        if (tokens[i].kind == "ID")
        {
            for (auto x : ids)
            {
                if (tokens[i].lexeme == x)
                {
                    temp++;
                }
            }
            if (temp == 0)
            {
                throw runtime_error("ERROR: invalid id\n");
            }
        }

        // check valid dotid
        if (tokens[i].kind == "DOTID")
        {
            if (tokens[i].lexeme != ".word")
            {
                throw runtime_error("ERROR: wrong dotid\n");
            }
        }

        // must start w label or instr
        if (start == 1 && tokens[i].kind != "NEWLINE")
        {
            if (tokens[i].kind != "LABELDEF" && tokens[i].kind != "ID" && tokens[i].kind != "DOTID")
            {
                throw runtime_error("ERROR: wrong order\n");
            }
            int a = i;
            if (tokens[i].kind == "LABELDEF")
            {
                while (a < tokens.size())
                {
                    if (tokens[a].kind != "LABELDEF" && tokens[a].kind != "NEWLINE")
                    {
                        break;
                    }
                    if (tokens[a].kind == "NEWLINE" || a == tokens.size() - 1)
                    {
                        no = 0;
                        break;
                    }
                    a++;
                }
            }
        }

        if (i != 0 && tokens[i].kind == "NEWLINE" && tokens[i - 1].kind != "NEWLINE")
        {
            start = 1;
            if (no)
            {
                line++;
                
            }
            no = 1;
        }
        else
        {
            start = 0;
        }

        // add labels to symboltable
        if (tokens[i].kind == "LABELDEF")
        {
            if (symboltable.find(tokens[i].lexeme.substr(0, tokens[i].lexeme.size() - 1)) != symboltable.end())
            {
                throw runtime_error("ERROR: duplicate label\n");
            }

            symboltable.insert({tokens[i].lexeme.substr(0, tokens[i].lexeme.size() - 1), line});
        }
        // check for .word
        if (tokens[i].kind == "DOTID" && tokens[i].lexeme == ".word")
        {
            if (i + 1 > tokens.size() - 1)
            {
                throw runtime_error("ERROR: invalid syntax\n");
            }
            if (
                tokens[i + 1].kind != "DECINT" &&
                tokens[i + 1].kind != "HEXINT" &&
                tokens[i + 1].kind != "ID")
            {
                throw runtime_error("ERROR: .word\n");
            }
            if (i + 1 != tokens.size() - 1 && tokens[i + 2].kind != "NEWLINE")
            {
                throw runtime_error("ERROR: invalid syntax\n");
            }
            i = i + 1;
        }
        // add, sub, slt, sltu syntax
        if (
            tokens[i].lexeme == "add" ||
            tokens[i].lexeme == "sub" ||
            tokens[i].lexeme == "slt" ||
            tokens[i].lexeme == "sltu")
        {
            if (i + 5 > tokens.size() - 1)
            {
                throw runtime_error("ERROR: invalid syntax\n");
            }
            if (
                tokens[i + 1].kind != "REGISTER" ||
                tokens[i + 2].kind != "COMMA" ||
                tokens[i + 3].kind != "REGISTER" ||
                tokens[i + 4].kind != "COMMA" ||
                tokens[i + 5].kind != "REGISTER")
            {
                throw runtime_error("ERROR: invalid syntax\n");
            }
            if (i + 5 != tokens.size() - 1 && tokens[i + 6].kind != "NEWLINE")
            {
                throw runtime_error("ERROR: invalid syntax\n");
            }
            i = i + 5;
        }
        // beq, bne
        if (
            tokens[i].lexeme == "beq" ||
            tokens[i].lexeme == "bne")
        {
            if (i + 5 > tokens.size() - 1)
            {
                throw runtime_error("ERROR: invalid syntax\n");
            }
            if (
                tokens[i + 1].kind != "REGISTER" ||
                tokens[i + 2].kind != "COMMA" ||
                tokens[i + 3].kind != "REGISTER" ||
                tokens[i + 4].kind != "COMMA")
            {
                throw runtime_error("ERROR: invalid syntax\n");
            }
            else
            {
                if (
                    tokens[i + 5].kind != "ID" &&
                    tokens[i + 5].kind != "DECINT" &&
                    tokens[i + 5].kind != "HEXINT")
                {
                    throw runtime_error("ERROR: invalid syntax\n");
                }
                if (tokens[i + 5].kind == "DECINT")
                {
                    if (!rangedec(tokens[i + 5].lexeme))
                    {
                        throw runtime_error("ERROR: range\n");
                    }
                }
                if (tokens[i + 5].kind == "HEXINT")
                {
                    if (!rangehex(tokens[i + 5].lexeme))
                    {
                        throw runtime_error("ERROR: range\n");
                    }
                }
            }
            if (i + 5 != tokens.size() - 1 && tokens[i + 6].kind != "NEWLINE")
            {
                throw runtime_error("ERROR: invalid syntax\n");
            }
            i = i + 5;
        }
        // mult, multu, div, divu
        if (
            tokens[i].lexeme == "mult" ||
            tokens[i].lexeme == "multu" ||
            tokens[i].lexeme == "div" ||
            tokens[i].lexeme == "divu")
        {
            if (i + 3 > tokens.size() - 1)
            {
                throw runtime_error("ERROR: invalid syntax\n");
            }
            if (
                tokens[i + 1].kind != "REGISTER" ||
                tokens[i + 2].kind != "COMMA" ||
                tokens[i + 3].kind != "REGISTER")
            {
                throw runtime_error("ERROR: invalid syntax\n");
            }
            if (i + 3 != tokens.size() - 1 && tokens[i + 4].kind != "NEWLINE")
            {
                throw runtime_error("ERROR: invalid syntax\n");
            }
            i = i + 3;
        }
        // mfhi, mflo, lis, jr, jalr
        if (
            tokens[i].lexeme == "mflo" ||
            tokens[i].lexeme == "mfhi" ||
            tokens[i].lexeme == "lis" ||
            tokens[i].lexeme == "jr" ||
            tokens[i].lexeme == "jalr")
        {
            if (i + 1 > tokens.size() - 1)
            {
                throw runtime_error("ERROR: invalid syntax\n");
            }
            if (tokens[i + 1].kind != "REGISTER")
            {
                throw runtime_error("ERROR: invalid syntax\n");
            }
            if (i + 1 != tokens.size() - 1 && tokens[i + 2].kind != "NEWLINE")
            {
                throw runtime_error("ERROR: invalid syntax\n");
            }
            i = i + 1;
        }
        // lw, sw
        if (
            tokens[i].lexeme == "lw" ||
            tokens[i].lexeme == "sw")
        {
            if (i + 6 > tokens.size() - 1)
            {
                throw runtime_error("ERROR: invalid syntax\n");
            }
            if (
                tokens[i + 1].kind != "REGISTER" ||
                tokens[i + 2].kind != "COMMA" ||
                tokens[i + 4].kind != "LPAREN" ||
                tokens[i + 5].kind != "REGISTER" ||
                tokens[i + 6].kind != "RPAREN")
            {
                throw runtime_error("ERROR: invalid syntax\n");
            }
            else
            {
                if (
                    tokens[i + 3].kind != "DECINT" &&
                    tokens[i + 3].kind != "HEXINT")
                {
                    throw runtime_error("ERROR: invalid syntax\n");
                }
                if (tokens[i + 3].kind == "DECINT")
                {
                    if (!rangedec(tokens[i + 3].lexeme))
                    {
                        throw runtime_error("ERROR: range\n");
                    }
                }
                if (tokens[i + 3].kind == "HEXINT")
                {
                    if (!rangehex(tokens[i + 3].lexeme))
                    {
                        throw runtime_error("ERROR: range\n");
                    }
                }
                if (i + 6 != tokens.size() - 1 && tokens[i + 7].kind != "NEWLINE")
                {
                    throw runtime_error("ERROR: invalid syntax\n");
                }
                i = i + 6;
            }
        }
    }

    return symboltable;
}

void secondpass(vector<Token> tokens, map<string, int> symboltable)
{
    int machinecode = 0;
    int line = 0;
    int start = 1;
    int no = 1;

    map<string, int> bits = {
        {"add", 0x20},
        {"sub", 0x22},
        {"slt", 0x2a},
        {"sltu", 0x2b},
        {"mult", 0x18},
        {"multu", 0x19},
        {"div", 0x1a},
        {"divu", 0x1b},
        {"mfhi", 0x10},
        {"mflo", 0x12},
        {"lis", 0x14},
        {"jr", 0x08},
        {"jalr", 0x09},
        {"beq", 0x10},
        {"bne", 0x14},
        {"lw", 0x8c},
        {"sw", 0xac}};

    for (int i = 0; i < tokens.size(); i++)
    {
        machinecode = 0;

        // must start w label or instr
        if (start == 1 && tokens[i].kind != "NEWLINE")
        {
            int a = i;
            if (tokens[i].kind == "LABELDEF")
            {
                while (a < tokens.size())
                {
                    if (tokens[a].kind != "LABELDEF" && tokens[a].kind != "NEWLINE")
                    {
                        break;
                    }
                    if (tokens[a].kind == "NEWLINE" || a == tokens.size() - 1)
                    {
                        no = 0;
                        break;
                    }
                    a++;
                }
            }
        }

        if (i != 0 && tokens[i].kind == "NEWLINE" && tokens[i - 1].kind != "NEWLINE")
        {
            start = 1;
            if (no)
            {
                line++;
            }
            no = 1;
        }
        else
        {
            start = 0;
        }

        if (tokens[i].lexeme == ".word")
        {
            long long val = 0;
            if (tokens[i + 1].kind == "ID")
            {
                if (symboltable.find(tokens[i + 1].lexeme) != symboltable.end())
                {
                    val = (symboltable[tokens[i + 1].lexeme]) * 4;
                }
                else
                {
                    throw runtime_error("ERROR: undefined label\n");
                }
            }
            if (tokens[i + 1].kind == "DECINT")
            {
                val = errordec(tokens[i + 1].lexeme);
            }
            if (tokens[i + 1].kind == "HEXINT")
            {
                val = errorhex(tokens[i + 1].lexeme);
            }

            machinecode = val;
            printmachinecode(machinecode);

            i = i + 1;
        }
        if (
            tokens[i].lexeme == "add" ||
            tokens[i].lexeme == "sub" ||
            tokens[i].lexeme == "slt" ||
            tokens[i].lexeme == "sltu")
        {
            machinecode =
                ((stoi(tokens[i + 3].lexeme.substr(1)) & 0x1F) << 21) |
                ((stoi(tokens[i + 5].lexeme.substr(1)) & 0x1F) << 16) |
                ((stoi(tokens[i + 1].lexeme.substr(1)) & 0x1F) << 11);

            machinecode |= (bits[tokens[i].lexeme] & 0xFF);

            printmachinecode(machinecode);

            i = i + 5;
        }
        if (
            tokens[i].lexeme == "beq" ||
            tokens[i].lexeme == "bne")
        {
            long long int offset = 0;
            if (tokens[i + 5].kind == "ID")
            {
                if (symboltable.find(tokens[i + 5].lexeme) != symboltable.end())
                {
                    offset = (symboltable[tokens[i + 5].lexeme]) - (line + 1);
                    if (offset > 32767 || offset < -32768)
                    {
                        throw runtime_error("ERROR: range\n");
                    }
                }
                else
                {
                    throw runtime_error("ERROR: label undefined\n");
                }
            }
            if (tokens[i + 5].kind == "DECINT")
            {
                offset = errordec(tokens[i+5].lexeme);

            }
            if (tokens[i + 5].kind == "HEXINT")
            {
                offset = errorhex(tokens[i+5].lexeme);
            }

            machinecode =
                ((stoi(tokens[i + 1].lexeme.substr(1)) & 0x1F) << 21) |
                ((stoi(tokens[i + 3].lexeme.substr(1)) & 0x1F) << 16) |
                (offset & 0xFFFF);

            machinecode |= (bits[tokens[i].lexeme] << 24);
            printmachinecode(machinecode);

            i = i + 5;
        }

        if (
            tokens[i].lexeme == "mult" ||
            tokens[i].lexeme == "multu" ||
            tokens[i].lexeme == "div" ||
            tokens[i].lexeme == "divu")
        {
            machinecode =
                (stoi(tokens[i + 1].lexeme.substr(1)) << 21) |
                (stoi(tokens[i + 3].lexeme.substr(1)) << 16);

            machinecode |= bits[tokens[i].lexeme];
            printmachinecode(machinecode);

            i = i + 3;
        }
        if (
            tokens[i].lexeme == "mflo" ||
            tokens[i].lexeme == "mfhi" ||
            tokens[i].lexeme == "lis")
        {
            machinecode |= ((stoi(tokens[i + 1].lexeme.substr(1)) & 0x1F) << 11);
            machinecode |= (bits[tokens[i].lexeme] & 0xFF);
            printmachinecode(machinecode);

            i = i + 1;
        }
        if (
            tokens[i].lexeme == "jr" ||
            tokens[i].lexeme == "jalr")
        {
            machinecode = (stoi(tokens[i + 1].lexeme.substr(1)) << 21);

            machinecode |= bits[tokens[i].lexeme];
            printmachinecode(machinecode);

            i = i + 1;
        }
        if (
            tokens[i].lexeme == "lw" ||
            tokens[i].lexeme == "sw")
        {
            long long int offset = 0;

            if (tokens[i + 3].kind == "DECINT")
            {
                offset = errordec(tokens[i+3].lexeme);
            }
            if (tokens[i + 3].kind == "HEXINT")
            {
                offset = errorhex(tokens[i+3].lexeme);
            }
            machinecode =
                ((stoi(tokens[i + 5].lexeme.substr(1)) & 0x1F) << 21) |
                ((stoi(tokens[i + 1].lexeme.substr(1)) & 0x1F) << 16) |
                (offset & 0xFFFF);

            machinecode |= (bits[tokens[i].lexeme] << 24);
            printmachinecode(machinecode);

            i = i + 6;
        }
        machinecode = 0;
    }
}

long long int errordec(string number)
{
    long long int val = 0;
    try
    {
        val = std::stoll(number);
    }
    catch (const std::exception &e)
    {
        cerr << "ERROR" << endl;
    }
    return val;
}

long long int errorhex(string number)
{
    long long int val = 0;
    try
    {
        val = std::stoll(number.substr(2), 0, 16);
    }
    catch (const std::exception &e)
    {
        cerr << "ERROR" << endl;
    }
    return val;
}

bool rangedec(string dec)
{
    long long int decval = std::stoll(dec);
    if (decval > 32767 || decval < -32768)
    {
        return 0;
    }
    return 1;
}

bool rangehex(string hex)
{
    long long int hexval = std::stoll(hex.substr(2), 0, 16);
    if (hexval > 65535)
    {
        return 0;
    }
    return 1;
}

void printmachinecode(int machinecode)
{
    // since 4 bytes (32 bits)
    putchar((machinecode >> 24) & 0x00FF);
    putchar((machinecode >> 16) & 0x00FF);
    putchar((machinecode >> 8) & 0x00FF);
    putchar(machinecode & 0x00FF);
}

//// Helper functions

bool isChar(string s)
{
    return s.length() == 1;
}

bool isRange(string s)
{
    return s.length() == 3 && s[1] == '-';
}

string squish(string s)
{
    stringstream ss(s);
    string token;
    string result;
    string space = "";
    while (ss >> token)
    {
        result += space;
        result += token;
        space = " ";
    }
    return result;
}

int hexToNum(char c)
{
    if ('0' <= c && c <= '9')
    {
        return c - '0';
    }
    else if ('a' <= c && c <= 'f')
    {
        return 10 + (c - 'a');
    }
    else if ('A' <= c && c <= 'F')
    {
        return 10 + (c - 'A');
    }
    // This should never happen....
    throw runtime_error("ERROR: Invalid hex digit!");
}

char numToHex(int d)
{
    return (d < 10 ? d + '0' : d - 10 + 'A');
}

string escape(string s)
{
    string p;
    for (int i = 0; i < s.length(); ++i)
    {
        if (s[i] == '\\' && i + 1 < s.length())
        {
            char c = s[i + 1];
            i = i + 1;
            if (c == 's')
            {
                p += ' ';
            }
            else if (c == 'n')
            {
                p += '\n';
            }
            else if (c == 'r')
            {
                p += '\r';
            }
            else if (c == 't')
            {
                p += '\t';
            }
            else if (c == 'x')
            {
                if (i + 2 < s.length() && isxdigit(s[i + 1]) && isxdigit(s[i + 2]))
                { // WHY CHECK i+2???????????????????????? x : i, 4 : i+1 // WHY IS TRANSITION HEXA?
                    if (hexToNum(s[i + 1]) > 8)
                    { ////////////////////////////////////////// why is this 8
                        throw runtime_error(
                            "ERROR: Invalid escape sequence \\x" + string(1, s[i + 1]) + string(1, s[i + 2]) + ": not in ASCII range (0x00 to 0x7F)");
                    }
                    char code = hexToNum(s[i + 1]) * 16 + hexToNum(s[i + 2]);
                    p += code;
                    i = i + 2;
                }
                else
                {
                    p += c;
                }
            }
            else if (isgraph(c))
            {
                p += c;
            }
            else
            {
                p += s[i];
            }
        }
        else
        {
            p += s[i];
        }
    }
    return p;
}

string unescape(string s)
{
    string p;
    for (int i = 0; i < s.length(); ++i)
    {
        char c = s[i];
        if (c == ' ')
        {
            p += "\\s";
        }
        else if (c == '\n')
        {
            p += "\\n";
        }
        else if (c == '\r')
        {
            p += "\\r";
        }
        else if (c == '\t')
        {
            p += "\\t";
        }
        else if (!isgraph(c))
        {
            string hex = "\\x";
            p += hex + numToHex((unsigned char)c / 16) + numToHex((unsigned char)c % 16);
        }
        else
        {
            p += c;
        }
    }
    return p;
}

int main()
{
    // create ur dfa
    mipsscan mips;
    DFA *dfa = nullptr;

    try
    {
        stringstream s(DFAstring);
        // call smm function to get tokens
        dfa = createDFA(s);

        // tokens
        vector<Token> final_tokens = mips.simplifiedMaximalMunch(dfa, cin);

        // assembler
        map<string, int> symboltable = firstpass(final_tokens);

        // std::map<std::string, int>::iterator it = symboltable.begin();
        // // Iterate through the symboltable and print the elements
        // while (it != symboltable.end())
        // {
        //     std::cout << "Key: " << it->first << ", Value: " << it->second << std::endl;
        //     ++it;
        // }

        secondpass(final_tokens, symboltable);

        delete dfa;
    }
    catch (runtime_error &e)
    {
        // delete dfa
        delete dfa;
        // error out
        cerr << e.what() << endl;
    }
    return 0;
}
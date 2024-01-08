#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <map>
#include <deque>
#include <algorithm>
#include "dfa.h"
#include "wlp4data.h"
#include "mipshelper.h"
// #include "wlp4data.cc"
// #include "mipshelper.cc"

using namespace std;

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

const string STATES = ".STATES";
const string TRANSITIONS = ".TRANSITIONS";
const string INPUT = ".INPUT";

//// TOKEN /////////////////////////////////////////////////////////
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

class wlp4scan
{
public:
    deque<Token> simplifiedMaximalMunch(DFA *dfa, istream &inputs)
    {
        string s;
        string lex;
        deque<Token> tokens;
        string currentState = "start";
        string temp;
        string line_str;
        // int force = 0;

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

            // if (force != 0)
            // {
            //     Token newToken{"NEWLINE", ""};
            //     tokens.push_back(newToken);
            // }

            // force++;

            for (int i = 0; i < s.length(); i++)
            {
                char c = s[i];
                temp = currentState;
                currentState = getNextState(dfa, temp, c);

                if (currentState.empty())
                {
                    if ((*dfa->accepting)[temp] == 0)
                    {
                        throw runtime_error("ERROR");
                    }
                    if (temp == "ZERO")
                    {
                        temp = "NUM";
                    }
                    if (temp == "ID")
                    {
                        if (lex == "int")
                        {
                            temp = "INT";
                        }
                        if (lex == "wain")
                        {
                            temp = "WAIN";
                        }
                        if (lex == "if")
                        {
                            temp = "IF";
                        }
                        if (lex == "else")
                        {
                            temp = "ELSE";
                        }
                        if (lex == "while")
                        {
                            temp = "WHILE";
                        }
                        if (lex == "println")
                        {
                            temp = "PRINTLN";
                        }
                        if (lex == "return")
                        {
                            temp = "RETURN";
                        }
                        if (lex == "new")
                        {
                            temp = "NEW";
                        }
                        if (lex == "delete")
                        {
                            temp = "DELETE";
                        }
                        if (lex == "NULL")
                        {
                            temp = "NULL";
                        }
                    }
                    if (temp == "NUM")
                    {
                        try
                        {
                            long long num2 = std::stoll(lex);
                            if (num2 > 2147483647)
                            {
                                throw runtime_error("ERROR: NUM too big");
                            }
                        }
                        catch (const std::exception &e)
                        {
                            throw runtime_error("ERROR: NUM too big");
                        }
                    }

                    if (temp == "ZERO")
                    {
                        temp = "NUM";
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

//// Helper functions /////////////////////////////////////////////
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

//// PARSING /////////////////////////////////////////////////////
struct Rules
{
    string lhs;
    vector<string> rhs;
};

struct Slr
{
    // transition : key - <state, symbol> value - state
    map<pair<string, string>, string> transitions;
    // reductions : key - <state, symbol> value - rule-number
    map<pair<string, string>, string> reductions;
};

struct TreeNode
{
    string tokenvrule;
    string type;
    Token token;
    Rules rule;
    vector<TreeNode *> children;

    ~TreeNode()
    {
        for (TreeNode *child : children)
        {
            delete child;
        }
    }
};

void populate_cfg(istream &in, vector<Rules> &cfg)
{
    string s;
    string word;

    // read line ".CFG"
    getline(in, s);

    Rules rule;
    while (getline(in, s))
    {
        Rules rule;
        istringstream iss(s);

        iss >> word;

        rule.lhs = word;

        while (iss >> word)
        {
            if (word == ".EMPTY")
            {
                break;
            }
            rule.rhs.push_back(word);
        }

        cfg.push_back(rule);
        rule.rhs.clear();
    }
}

void populate_slr(istream &trans, istream &reduce, Slr &slr)
{
    string t;
    string r;
    string first;
    string second;
    string third;

    // transitions
    // read line ".TRANSITIONS"
    getline(trans, t);

    while (getline(trans, t))
    {
        istringstream iss(t);

        iss >> first;
        iss >> second;
        iss >> third;

        slr.transitions[{first, second}] = third;
    }

    // reductions
    // read line ".REDUCTIONS"
    getline(reduce, r);

    while (getline(reduce, r))
    {
        istringstream iss(r);

        iss >> first;
        iss >> second;
        iss >> third;

        slr.reductions[{first, third}] = second;
    }
}

void reduceTree(Rules rule, vector<TreeNode *> &tree_stack)
{
    // Create a new tree node storing the CFG rule.
    TreeNode *new_node = new TreeNode;
    new_node->tokenvrule = "rule";
    new_node->rule = rule;

    // Let len be the length of the right-hand side of the CFG rule.
    int len = rule.rhs.size();

    // Copy the last len trees from the tree stack into the new node's children.
    // Make sure to copy them in the right order.
    for (int i = 0; i < len; i++)
    {
        new_node->children.push_back(tree_stack.back());
        tree_stack.pop_back();
    }

    reverse(new_node->children.begin(), new_node->children.end());

    // Push the new node to the tree stack.
    tree_stack.push_back(new_node);
}

void reduceStates(Rules rule, vector<string> &state_stack, Slr slr)
{
    int len = rule.rhs.size();
    string state;

    for (int i = 0; i < len; i++)
    {
        state_stack.pop_back();
    }

    state = slr.transitions[{state_stack.back(), rule.lhs}];
    state_stack.push_back(state);
}

void shift(deque<Token> &tokens, vector<TreeNode *> &tree_stack, vector<string> &state_stack, Slr slr)
{
    TreeNode *node = new TreeNode;
    string state;

    node->token = tokens.front();
    node->tokenvrule = "token";

    state = slr.transitions[{state_stack.back(), tokens.front().kind}];
    if (state == "")
    {
        throw runtime_error("ERROR");
    }
    state_stack.push_back(state);
    tree_stack.push_back(node);

    tokens.pop_front();
}

void tokensToTrees(deque<Token> &tokens, vector<Rules> cfg, vector<TreeNode *> &tree_stack, vector<string> &state_stack, Slr slr)
{
    string reduce = "";
    string transition;
    int ruleno = 0;

    while (!tokens.empty())
    {
        // check reduce
        while (slr.reductions[{state_stack.back(), tokens.front().kind}] != "")
        {
            reduce = slr.reductions[{state_stack.back(), tokens.front().kind}];
            ruleno = stoi(reduce);

            reduceStates(cfg[ruleno], state_stack, slr);
            reduceTree(cfg[ruleno], tree_stack);
        }
        transition = slr.transitions[{state_stack.back(), tokens.front().kind}];
        if (transition != "")
        {
            shift(tokens, tree_stack, state_stack, slr);
        }
        else if (tokens.front().lexeme == ".ACCEPT")
        {
            tokens.pop_front();
        }
        else
        {
            throw runtime_error("ERROR");
        }
    }
}

void printRoot(TreeNode *root)
{
    if (root->tokenvrule == "rule")
    {
        cout << root->rule.lhs << " ";
        if (root->rule.rhs.empty())
        {
            cout << ".EMPTY";
        }
        else
        {
            for (const auto &symbol : root->rule.rhs)
            {
                cout << symbol << " ";
            }
        }
        cout << endl;
    }
    else if (root->tokenvrule == "token")
    {
        cout << root->token.kind << " " << root->token.lexeme << endl;
    }

    for (const auto &i : root->children)
    {
        printRoot(i);
    }
}

void printTree(vector<TreeNode *> tree_stack)
{
    for (const auto &i : tree_stack)
    {
        printRoot(i);
    }
}

void clean(vector<TreeNode *> tree_stack)
{
    for (auto child : tree_stack)
    {
        delete child;
    }
}

//// SEMANTIC ANALYSIS ///////////////////////////////////////////
TreeNode *getChild(TreeNode *root, string node, int count)
{
    for (auto i : root->children)
    {
        if (i->rule.lhs == node || i->token.kind == node)
        {
            count--;
            if (count == 0)
            {
                return i;
            }
        }
    }
    return root;
}

struct Variable
{
    string name;
    string type;

    Variable()
    {
        name = "";
        type = "";
    }

    Variable(TreeNode *root)
    {
        if (root->rule.lhs == "dcl")
        {
            if (root->children[0]->children.size() == 1)
            {
                type = "int";
            }
            else
            {
                type = "int*";
            }
            name = root->children[1]->token.lexeme;
        }
        else
        {
            name = "";
            type = "";
        }
    }
};

struct VariableTable
{
    map<string, Variable> varMap;

    void add(Variable var)
    {
        if (varMap[var.name].name != "")
        {
            throw runtime_error("ERROR: duplicate variable declaration");
        }
        varMap[var.name] = var;
    }

    Variable get(string name)
    {
        if (varMap[name].name == "")
        {
            throw runtime_error("ERROR: use of undeclared variable");
        }
        return varMap[name];
    }
};

struct Procedure
{
    string name;
    vector<string> signature;
    VariableTable localTable;

    Procedure()
    {
        name = "";
    }
    Procedure(TreeNode *root)
    {
        if (root->rule.lhs == "main")
        {
            name = "wain";
            // params
            Variable var1 = Variable(getChild(root, "dcl", 1));
            Variable var2 = Variable(getChild(root, "dcl", 2));
            if (var1.name != "")
            {
                signature.push_back(var1.type);
                localTable.add(var1);

                if (var2.name != "")
                {
                    if (var2.type != "int")
                    {
                        throw runtime_error("ERROR: second dcl from main must be INT");
                    }
                    signature.push_back(var2.type);
                    localTable.add(var2);
                }
            }
            else
            {
                signature.clear();
            }
        }
        else if (root->rule.lhs == "procedure")
        {
            name = getChild(root, "ID", 1)->token.lexeme;
            // params
            // rule is params : .EMPTY
            if (getChild(root, "params", 1)->rule.rhs.empty())
            {
                signature.clear();
            }
            else
            {
                TreeNode *paramlist = getChild(root, "params", 1)->children[0];
                while (paramlist->children.size() > 1)
                {
                    // rule is pramlist : dcl COMMA paramlist

                    Variable paramVar = Variable(getChild(paramlist, "dcl", 1));
                    signature.push_back(paramVar.type);
                    localTable.add(paramVar);

                    // change paramlist pointer to paramlist in the paramlist rule
                    paramlist = getChild(paramlist, "paramlist", 1);
                }
                // if rule is paramlist : dcl
                if (paramlist->children.size() == 1)
                {
                    Variable paramVar = Variable(getChild(paramlist, "dcl", 1));
                    signature.push_back(paramVar.type);
                    localTable.add(paramVar);
                }
            }
        }
        root = getChild(root, "dcls", 1);

        // local vars
        // root node is currently at dcls
        while (!root->rule.rhs.empty() && root->rule.rhs[0] == "dcls")
        {
            Variable locals = Variable(getChild(root, "dcl", 1));
            if (root->rule.rhs[3] == "NUM" && locals.type != "int")
            {
                throw runtime_error("ERROR: incorrect type for NUM assignment");
            }
            if (root->rule.rhs[3] == "NULL" && locals.type != "int*")
            {
                throw runtime_error("ERROR: incorrect type for NULL assignment");
            }
            localTable.add(locals);
            root = getChild(root, "dcls", 1);
            ;
        }
    }
};

struct ProcedureTable
{
    map<string, Procedure> procedureMap;

    void add(Procedure method)
    {
        if (procedureMap[method.name].name != "")
        {
            throw runtime_error("ERROR: duplicate procedure declaration");
        }
        procedureMap[method.name] = method;
    }

    Procedure get(string methodName)
    {
        if (procedureMap[methodName].name == "")
        {
            throw runtime_error("ERROR: use of undeclared procedure");
        }
        return procedureMap[methodName];
    }
};

void nodeAtStatements(TreeNode *statements, Procedure curent, ProcedureTable allProcedures);

void annotateNonterms(TreeNode *root, Procedure current, ProcedureTable allProcedures)
{
    if (root->rule.lhs == "expr")
    {
        if (root->rule.rhs[0] == "term")
        {
            annotateNonterms(getChild(root, "term", 1), current, allProcedures);
            root->type = getChild(root, "term", 1)->type;
        }
        else
        {
            TreeNode *secondArg = getChild(root, "term", 1);
            annotateNonterms(secondArg, current, allProcedures);
            TreeNode *firstArg = getChild(root, "expr", 1);
            annotateNonterms(firstArg, current, allProcedures);

            if (firstArg->type == "int" && secondArg->type == "int")
            {
                root->type = "int";
            }
            if (firstArg->type == "int*" && secondArg->type == "int")
            {
                root->type = "int*";
            }
            if (firstArg->type == "int" && secondArg->type == "int*")
            {
                if (root->rule.rhs[1] == "PLUS")
                {
                    root->type = "int*";
                }
                else
                {
                    throw runtime_error("ERROR: invalid operation for MINUS");
                }
            }
            if (firstArg->type == "int*" && secondArg->type == "int*")
            {
                if (root->rule.rhs[1] == "MINUS")
                {
                    root->type = "int";
                }
                else
                {
                    throw runtime_error("ERROR: invalid operation for PLUS");
                }
            }
        }
    }
    else if (root->rule.lhs == "term")
    {
        if (root->rule.rhs[0] == "factor")
        {
            annotateNonterms(getChild(root, "factor", 1), current, allProcedures);
            root->type = getChild(root, "factor", 1)->type;
        }
        if (root->rule.rhs[0] == "term")
        {
            TreeNode *secondArg = getChild(root, "factor", 1);
            annotateNonterms(secondArg, current, allProcedures);

            TreeNode *firstArg = getChild(root, "term", 1);
            annotateNonterms(firstArg, current, allProcedures);

            if (firstArg->type != "int" || secondArg->type != "int")
            {
                throw runtime_error("ERROR: term should be int in this arg");
            }
            root->type = "int";
        }
    }
    else if (root->rule.lhs == "factor")
    {
        if (root->rule.rhs[0] == "ID" && root->rule.rhs.size() == 1)
        {
            Variable local = current.localTable.get(getChild(root, "ID", 1)->token.lexeme);
            root->type = local.type;
        }
        if (root->rule.rhs[0] == "NUM")
        {
            root->type = "int";
        }
        if (root->rule.rhs[0] == "NULL")
        {
            root->type = "int*";
        }
        if (root->rule.rhs[0] == "LPAREN")
        {
            annotateNonterms(getChild(root, "expr", 1), current, allProcedures);
            root->type = getChild(root, "expr", 1)->type;
        }
        if (root->rule.rhs[0] == "AMP")
        {
            annotateNonterms(getChild(root, "lvalue", 1), current, allProcedures);
            if (getChild(root, "lvalue", 1)->type != "int")
            {
                throw runtime_error("ERROR: for factor rule after AMP should be type int");
            }
            root->type = "int*";
        }
        if (root->rule.rhs[0] == "STAR")
        {
            annotateNonterms(getChild(root, "factor", 1), current, allProcedures);
            if (getChild(root, "factor", 1)->type != "int*")
            {
                throw runtime_error("ERROR: for factor rule after STAR should be type int*");
            }
            root->type = "int";
        }
        if (root->rule.rhs[0] == "NEW")
        {
            annotateNonterms(getChild(root, "expr", 1), current, allProcedures);
            if (getChild(root, "expr", 1)->type != "int")
            {
                throw runtime_error("ERROR: for factor rule expr should be int");
            }
            root->type = "int*";
        }
        if (root->rule.rhs.front() == "ID" && root->rule.rhs.back() == "RPAREN")
        {
            Procedure methodCall = allProcedures.get(getChild(root, "ID", 1)->token.lexeme);
            Variable check = current.localTable.varMap[getChild(root, "ID", 1)->token.lexeme];
            if (check.name == getChild(root, "ID", 1)->token.lexeme)
            {
                throw runtime_error("ERROR: method name overlap with variable name");
            }

            if (root->rule.rhs.size() == 3)
            {
                if (!methodCall.signature.empty())
                {
                    throw runtime_error("ERROR: procedure call params are empty");
                }
            }
            if (root->rule.rhs.size() == 4)
            {
                TreeNode *arglist = getChild(root, "arglist", 1);
                vector<string> methodCallParam;

                while (arglist->children.size() == 3)
                {
                    annotateNonterms(getChild(arglist, "expr", 1), current, allProcedures);
                    methodCallParam.push_back(getChild(arglist, "expr", 1)->type);
                    arglist = arglist->children[2];
                }
                if (arglist->rule.rhs.size() == 1)
                {
                    annotateNonterms(getChild(arglist, "expr", 1), current, allProcedures);
                    methodCallParam.push_back(getChild(arglist, "expr", 1)->type);
                }

                if (methodCall.signature.size() != methodCallParam.size())
                {
                    throw runtime_error("ERROR: incorrect signature call");
                }
                for (int i = 0; i < methodCallParam.size(); i++)
                {
                    if (methodCallParam[i] != methodCall.signature[i])
                    {
                        throw runtime_error("ERROR: incorrect signature field");
                    }
                }
            }
            root->type = "int";
        }
    }
    else if (root->rule.lhs == "lvalue")
    {
        if (root->rule.rhs[0] == "ID")
        {
            Variable local = current.localTable.get(getChild(root, "ID", 1)->token.lexeme);
            root->type = local.type;
        }
        if (root->rule.rhs[0] == "STAR")
        {
            annotateNonterms(root->children[1], current, allProcedures);
            if (getChild(root, "factor", 1)->type != "int*")
            {
                throw runtime_error("ERROR: for factor rule after STAR should be type int*");
            }
            root->type = "int";
        }
        if (root->rule.rhs[0] == "LPAREN")
        {
            annotateNonterms(getChild(root, "lvalue", 1), current, allProcedures);
            root->type = getChild(root, "lvalue", 1)->type;
        }
    }
    else if (root->rule.lhs == "test")
    {
        TreeNode *firstArg = getChild(root, "expr", 1);
        annotateNonterms(getChild(root, "expr", 1), current, allProcedures);
        TreeNode *secondArg = getChild(root, "expr", 2);
        annotateNonterms(getChild(root, "expr", 2), current, allProcedures);

        if (firstArg->type != secondArg->type)
        {
            throw runtime_error("ERROR: type is not equal for test terminal");
        }
    }
    else
    {
        return;
    }
}

void annotateStatements(TreeNode *root, Procedure current, ProcedureTable allProcedures)
{
    // root is @ statement
    if (root->rule.rhs[0] == "lvalue")
    {
        TreeNode *firstArg = getChild(root, "lvalue", 1);
        annotateNonterms(firstArg, current, allProcedures);
        TreeNode *secondArg = getChild(root, "expr", 1);
        annotateNonterms(secondArg, current, allProcedures);

        if (firstArg->type != secondArg->type)
        {
            throw runtime_error("ERROR: type is not equivalent");
        }
    }
    else if (root->rule.rhs[0] == "IF")
    {
        annotateNonterms(getChild(root, "test", 1), current, allProcedures);
        nodeAtStatements(getChild(root, "statements", 1), current, allProcedures);
        nodeAtStatements(getChild(root, "statements", 2), current, allProcedures);
    }
    else if (root->rule.rhs[0] == "WHILE")
    {
        annotateNonterms(getChild(root, "test", 1), current, allProcedures);
        nodeAtStatements(getChild(root, "statements", 1), current, allProcedures);
    }
    else if (root->rule.rhs[0] == "PRINTLN")
    {
        annotateNonterms(getChild(root, "expr", 1), current, allProcedures);
        if (getChild(root, "expr", 1)->type != "int")
        {
            throw runtime_error("ERROR: incorrect PRINTLN type");
        }
    }
    else if (root->rule.rhs[0] == "DELETE")
    {
        annotateNonterms(getChild(root, "expr", 1), current, allProcedures);
        if (getChild(root, "expr", 1)->type != "int*")
        {
            throw runtime_error("ERROR: incorrect DELETE type");
        }
    }
    else
    {
        return;
    }
}

void nodeAtStatements(TreeNode *statements, Procedure current, ProcedureTable allProcedures)
{
    while (!statements->rule.rhs.empty() && statements->rule.rhs[0] == "statements")
    {
        annotateStatements(getChild(statements, "statement", 1), current, allProcedures);
        statements = getChild(statements, "statements", 1);
    }
}

void annotateTypes(TreeNode *method, Procedure current, ProcedureTable allProcedures)
{
    // type for expr, term, factor, lvalue
    TreeNode *traverse = method;
    traverse = getChild(method, "statements", 1);

    if (!traverse->rule.rhs.empty())
    {
        nodeAtStatements(traverse, current, allProcedures);
    }

    traverse = getChild(method, "expr", 1);
    annotateNonterms(traverse, current, allProcedures);
    if (traverse->type != "int")
    {
        throw runtime_error("ERROR: must return int");
    }
}

ProcedureTable collectProcedures(TreeNode *start)
{
    ProcedureTable table;
    start = getChild(start, "procedures", 1);
    while (start->rule.rhs[0] == "procedure")
    {
        Procedure method = Procedure(getChild(start, "procedure", 1));
        table.add(method);
        annotateTypes(getChild(start, "procedure", 1), method, table);
        start = getChild(start, "procedures", 1);
    }
    if (start->rule.rhs[0] == "main")
    {
        Procedure method = Procedure(getChild(start, "main", 1));
        table.add(method);
        annotateTypes(getChild(start, "main", 1), method, table);
    }

    return table;
}

//// CODE GENERATION //////////////////////////////////////////
void codeLvalue(TreeNode *root, map<string, int> offset_table);

void codeExpr(TreeNode *root, map<string, int> offset_table)
{
    if (root->rule.lhs == "expr")
    {
        if (root->rule.rhs[0] == "term")
        {
            codeExpr(getChild(root, "term", 1), offset_table);
        }
        else
        {
            codeExpr(getChild(root, "expr", 1), offset_table);
            push(3);
            codeExpr(getChild(root, "term", 1), offset_table);
            pop(5);
            if (root->children[1]->token.kind == "PLUS")
            {
                if (getChild(root, "expr", 1)->type == "int*")
                {
                    mult(3, 4);
                    mflo(3);
                }
                if (getChild(root, "term", 1)->type == "int*")
                {
                    mult(5, 4);
                    mflo(5);
                }
                add(3, 5, 3);
            }
            else if (root->children[1]->token.kind == "MINUS")
            {
                if (getChild(root, "expr", 1)->type == "int*" && getChild(root, "term", 1)->type == "int")
                {
                    mult(3, 4);
                    mflo(3);
                    sub(3, 5, 3);
                }
                else if (getChild(root, "expr", 1)->type == "int*" && getChild(root, "term", 1)->type == "int*")
                {
                    sub(3, 5, 3);
                    divide(3, 4);
                    mflo(3);
                }
                else
                {
                    sub(3, 5, 3);
                }
            }
        }
    }
    else if (root->rule.lhs == "term")
    {
        if (root->rule.rhs[0] == "factor")
        {
            codeExpr(getChild(root, "factor", 1), offset_table);
        }
        else
        {
            codeExpr(getChild(root, "term", 1), offset_table);
            push(3);
            codeExpr(getChild(root, "factor", 1), offset_table);
            pop(5);
            if (root->children[1]->token.kind == "STAR")
            {
                mult(3, 5);
                mflo(3);
            }
            else if (root->children[1]->token.kind == "SLASH")
            {
                divide(5, 3);
                mflo(3);
            }
            else if (root->children[1]->token.kind == "PCT")
            {
                divide(5, 3);
                mfhi(3);
            }
        }
    }
    else if (root->rule.lhs == "factor")
    {
        if (root->rule.rhs[0] == "ID" && root->rule.rhs.size() == 1)
        {
            int offset = offset_table[getChild(root, "ID", 1)->token.lexeme];
            lw(3, offset, 29);
        }
        else if (root->rule.rhs[0] == "NUM")
        {
            lis(3);
            word(getChild(root, "NUM", 1)->token.lexeme);
        }
        else if (root->rule.rhs[0] == "NULL")
        {
            lis(3);
            word(1);
        }
        else if (root->rule.rhs[0] == "LPAREN")
        {
            codeExpr(getChild(root, "expr", 1), offset_table);
        }
        else if (root->rule.rhs[0] == "AMP")
        {
            codeLvalue(getChild(root, "lvalue", 1), offset_table);
        }
        else if (root->rule.rhs[0] == "STAR")
        {
            codeExpr(getChild(root, "factor", 1), offset_table);
            lw(3, 0, 3);
        }
        else if (root->rule.rhs[0] == "NEW")
        {
            codeExpr(getChild(root, "expr", 1), offset_table);
            add(1, 0, 3);
            push(31);
            jalr(10);
            pop(31);
            bne(3, 0, to_string(1));
            add(3, 0, 11); // if $3 = 0 and new alloc failed, $3 = 1 aka null
            // if $3 !=0 and new alloc succeeds, returns $3 and goes to next instr
        }
        else if (root->rule.rhs[0] == "ID" && root->rule.rhs.back() == "RPAREN")
        {
            push(7);
            lis(7);
            word("P" + getChild(root, "ID", 1)->token.lexeme);
            if (root->rule.rhs.size() == 3)
            {
                push(31);
                push(29);
                jalr(7);
                pop(29);
                pop(31);
                pop(7);
            }
            if (root->rule.rhs.size() == 4)
            {
                push(31);
                push(29);
                TreeNode *arglist = getChild(root, "arglist", 1);
                int count = 0;
                while (arglist->children.size() == 3)
                {
                    codeExpr(getChild(arglist, "expr", 1), offset_table);
                    push(3);
                    arglist = arglist->children[2];
                    count++;
                }
                if (arglist->rule.rhs.size() == 1)
                {
                    codeExpr(getChild(arglist, "expr", 1), offset_table);
                    push(3);
                    count++;
                }
                jalr(7);
                for (int i = 0; i < count; i++)
                {
                    pop();
                }
                pop(29);
                pop(31);
                pop(7);
            }
        }
    }
}

void codeLvalue(TreeNode *root, map<string, int> offset_table)
{
    if (root->rule.rhs[0] == "ID")
    {
        int offset = offset_table[getChild(root, "ID", 1)->token.lexeme];
        lis(3);
        word(offset);
        add(3, 3, 29);
    }
    else if (root->rule.rhs[0] == "STAR")
    {
        codeExpr(getChild(root, "factor", 1), offset_table);
    }
    else if (root->rule.rhs[0] == "LPAREN")
    {
        codeLvalue(getChild(root, "lvalue", 1), offset_table);
    }
}

void codeTest(TreeNode *root, map<string, int> offset_table, string stm_kind, int globalcount)
{
    TreeNode *firstArg = getChild(root, "expr", 1);
    TreeNode *secondArg = getChild(root, "expr", 2);

    codeExpr(firstArg, offset_table);
    push(3);
    codeExpr(secondArg, offset_table);
    pop(5);

    string label = "";
    label = "after" + stm_kind + to_string(globalcount);

    if (root->rule.rhs[1] == "EQ")
    {
        bne(3, 5, label); // jump to else or after while loop
    }
    else if (root->rule.rhs[1] == "NE")
    {
        beq(3, 5, label);
    }
    else if (root->rule.rhs[1] == "LT" || root->rule.rhs[1] == "GT" || root->rule.rhs[1] == "LE" || root->rule.rhs[1] == "GE")
    {
        // how to know when to use slt? vs sltu?
        if (getChild(root, "expr", 1)->type == "int*" && root->rule.rhs[1] == "LT")
        {
            sltu(3, 5, 3); // if lhs<rhs : $3 = 1 ; if lhs>=rhs : $3 = 0
            beq(3, 0, label);
        }
        else if (getChild(root, "expr", 1)->type == "int*" && root->rule.rhs[1] == "LE")
        {
            sltu(3, 3, 5); // if lhs>rhs : $3 = 1 ; if lhs<=rhs : $3 = 0
            beq(3, 11, label);
        }
        else if (getChild(root, "expr", 1)->type == "int*" && root->rule.rhs[1] == "GT")
        {
            sltu(3, 3, 5); // if rhs<lhs : $3 = 1 ; if rhs>=lhs : $3 = 0
            beq(3, 0, label);
        }
        else if (getChild(root, "expr", 1)->type == "int*" && root->rule.rhs[1] == "GE")
        {
            sltu(3, 5, 3); // if lhs<rhs : $3 = 1 ; if lhs>=rhs : $3 = 0
            beq(3, 11, label);
        }
        else if (getChild(root, "expr", 1)->type == "int" && root->rule.rhs[1] == "LT")
        {
            slt(3, 5, 3); // if lhs<rhs : $3 = 1 ; if lhs>=rhs : $3 = 0
            beq(3, 0, label);
        }
        else if (getChild(root, "expr", 1)->type == "int" && root->rule.rhs[1] == "LE")
        {
            slt(3, 3, 5); // if rhs<lhs : $3 = 1 ; if lhs<=rhs : $3 = 0
            beq(3, 11, label);
        }
        else if (getChild(root, "expr", 1)->type == "int" && root->rule.rhs[1] == "GT")
        {
            slt(3, 3, 5); // if rhs<lhs : $3 = 1 ; if rhs>=lhs : $3 = 0
            beq(3, 0, label);
        }
        else if (getChild(root, "expr", 1)->type == "int" && root->rule.rhs[1] == "GE")
        {
            slt(3, 5, 3); // if lhs<rhs : $3 = 1 ; if lhs>=rhs : $3 = 0
            beq(3, 11, label);
        }
    }
}

void codeStatementsTOStatement(TreeNode *root, map<string, int> offset_table, int &globalifcount, int &globalwhilecount);

void codeStatement(TreeNode *root, map<string, int> offset_table, int &globalifcount, int &globalwhilecount)
{
    if (root->rule.rhs[0] == "lvalue")
    {
        codeLvalue(getChild(root, "lvalue", 1), offset_table);
        push(3);
        codeExpr(getChild(root, "expr", 1), offset_table);
        pop(5);
        sw(3, 0, 5);
    }
    else if (root->rule.rhs[0] == "PRINTLN")
    {
        codeExpr(getChild(root, "expr", 1), offset_table);
        add(1, 0, 3); // add to register $1 for print parameter
        push(31);     // save $31 (PC) for jalr
        jalr(13);
        pop(31);
    }
    else if (root->rule.rhs[0] == "IF")
    {
        int currentIfIndex = globalifcount;
        globalifcount++;
        codeTest(getChild(root, "test", 1), offset_table, "if", currentIfIndex);
        // code for if statements
        codeStatementsTOStatement(getChild(root, "statements", 1), offset_table, globalifcount, globalwhilecount);
        // after jump to after else (will not run else code)
        lis(14);
        word("afterelse" + to_string(currentIfIndex));
        jr(14);
        label("afterif" + to_string(currentIfIndex));
        // code for else statements
        codeStatementsTOStatement(getChild(root, "statements", 2), offset_table, globalifcount, globalwhilecount);
        label("afterelse" + to_string(currentIfIndex));
    }
    else if (root->rule.rhs[0] == "WHILE")
    {
        int currentWhileIndex = globalwhilecount;
        globalwhilecount++;
        label("while" + to_string(currentWhileIndex));
        codeTest(getChild(root, "test", 1), offset_table, "while", currentWhileIndex);
        // code for while statements
        codeStatementsTOStatement(getChild(root, "statements", 1), offset_table, globalifcount, globalwhilecount);
        lis(14);
        word("while" + to_string(currentWhileIndex));
        jr(14);
        label("afterwhile" + to_string(currentWhileIndex));
    }
    else if (root->rule.rhs[0] == "DELETE")
    {
        codeExpr(getChild(root, "expr", 1), offset_table);
        add(1, 0, 3);             // $1 will hold address of expr
        beq(1, 11, to_string(5)); // if $1 is NULL, should do nothing so skip delete instruction
        push(31);
        jalr(9);
        pop(31);
    }
}

void codeStatementsTOStatement(TreeNode *root, map<string, int> offset_table, int &globalifcount, int &globalwhilecount)
{
    if (root->rule.rhs.empty())
    {
        return;
    }
    else
    {
        codeStatementsTOStatement(getChild(root, "statements", 1), offset_table, globalifcount, globalwhilecount);
        codeStatement(getChild(root, "statement", 1), offset_table, globalifcount, globalwhilecount);
    }
}

void codeProcedure(TreeNode *root, int &globalifcount, int &globalwhilecount, Procedure method)
{
    int localvarCount = 0;
    map<string, int> proc_offset_table;
    label("P" + getChild(root, "ID", 1)->token.lexeme);
    int i = method.signature.size();
    TreeNode *params = getChild(root, "params", 1);
    if (!params->rule.rhs.empty() && params->rule.rhs[0] == "paramlist")
    {
        params = getChild(params, "paramlist", 1);
        while (params->rule.rhs.size() > 1)
        {
            proc_offset_table[getChild(params, "dcl", 1)->children[1]->token.lexeme] = i * 4;
            i--;
            params = getChild(params, "paramlist", 1);
        }
        proc_offset_table[getChild(params, "dcl", 1)->children[1]->token.lexeme] = i * 4;
        i--;
    }
    // params are pushed by caller
    // set up frame pointer
    sub(29, 30, 4);

    // push and set up local variables and offset table
    TreeNode *vars = getChild(root, "dcls", 1);
    while (vars->children.size() > 1)
    {
        proc_offset_table[getChild(vars, "dcl", 1)->children[1]->token.lexeme] = -4 * localvarCount;
        lis(5);
        if (vars->children[3]->token.kind == "NULL")
        {
            word(1);
        }
        else if (vars->children[3]->token.kind == "NUM")
        {
            word(getChild(vars, "NUM", 1)->token.lexeme);
        }
        push(5);
        localvarCount++;
        vars = getChild(vars, "dcls", 1);
    }

    codeStatementsTOStatement(getChild(root, "statements", 1), proc_offset_table, globalifcount, globalwhilecount);

    // return expr
    root = getChild(root, "expr", 1);
    codeExpr(root, proc_offset_table);

    // clean up stack and return
    for (int i = 0; i < localvarCount; i++)
    {
        pop();
    }
    jr(31);
}

void codegen(TreeNode *start, ProcedureTable table)
{
    cout << ".import print\n";
    cout << ".import init\n.import new\n.import delete\n";
    lis(13); // $13 has label for print procedure
    word("print");
    lis(12); // $12 has label init
    word("init");
    lis(10); // $10 has label new
    word("new");
    lis(9); // $9 has label delete
    word("delete");
    lis(4); // load 4 into $4
    word(4);
    lis(11); // load 1 into $11
    word(1);
    lis(6);
    word("wain");
    jr(6);

    int localvarCount = 0;
    int globalifcount = 0;
    int globalwhilecount = 0;

    start = getChild(start, "procedures", 1);

    // traverse through all procedures
    while (start->children.size() > 1)
    {
        Procedure method = table.get(getChild(start, "procedure", 1)->children[1]->token.lexeme);
        codeProcedure(getChild(start, "procedure", 1), globalifcount, globalwhilecount, method);
        start = getChild(start, "procedures", 1);
    }

    // traverse node to main node
    start = getChild(start, "main", 1);

    label("wain");

    // push parameter vars
    map<string, int> wain_offset_table;
    wain_offset_table[getChild(start, "dcl", 1)->children[1]->token.lexeme] = 8;
    wain_offset_table[getChild(start, "dcl", 2)->children[1]->token.lexeme] = 4;
    push(1); // push register $1 (parameter 1)
    push(2); // push register $2 (parameter 2)

    // for initialization
    Procedure wainProcedure = table.get("wain");
    push(31);
    if (wainProcedure.signature[0] == "int")
    {
        push(2);
        add(2, 0, 0);
        jalr(12);
        pop(2);
    }
    else
    {
        jalr(12);
    }
    pop(31);

    // set up frame pointer
    sub(29, 30, 4);

    // push local vars
    TreeNode *vars = getChild(start, "dcls", 1);

    while (vars->children.size() > 1)
    {
        wain_offset_table[getChild(vars, "dcl", 1)->children[1]->token.lexeme] = -4 * localvarCount;
        lis(5);
        if (vars->children[3]->token.kind == "NULL")
        {
            word(1);
        }
        else
        {
            word(vars->children[3]->token.lexeme);
        }
        push(5);
        localvarCount++;
        vars = getChild(vars, "dcls", 1);
    }

    // statements
    codeStatementsTOStatement(getChild(start, "statements", 1), wain_offset_table, globalifcount, globalwhilecount);

    // return expr
    start = getChild(start, "expr", 1);
    codeExpr(start, wain_offset_table);

    // clean up stack and return
    for (int i = 0; i < localvarCount; i++)
    {
        pop();
    }
    jr(31);
}

int main()
{
    // create ur dfa
    wlp4scan wlp;
    DFA *dfa = nullptr;
    vector<TreeNode *> tree_stack;

    try
    {
        stringstream s(DFAstring);
        // call smm function to get tokens
        dfa = createDFA(s);
        // print tokens
        deque<Token> final_tokens = wlp.simplifiedMaximalMunch(dfa, cin);

        // aguement input
        Token beg{"BOF", "BOF"};
        final_tokens.push_front(beg);
        Token end{"EOF", "EOF"};
        final_tokens.push_back(end);
        Token accept{".ACCEPT", ".ACCEPT"};
        final_tokens.push_back(accept);

        // populate cfg, transitions, reductions
        stringstream a(WLP4_CFG);
        vector<Rules> cfg;
        populate_cfg(a, cfg);
        stringstream c(WLP4_TRANSITIONS);
        stringstream b(WLP4_REDUCTIONS);
        Slr slr1;
        populate_slr(c, b, slr1);

        // set up stack
        vector<string> state_stack;
        state_stack.push_back("0");
        tokensToTrees(final_tokens, cfg, tree_stack, state_stack, slr1);

        ProcedureTable table = collectProcedures(tree_stack[0]);

        codegen(tree_stack[0], table);
        // printTree(tree_stack);

        clean(tree_stack);
        delete dfa;
    }
    catch (runtime_error &e)
    {
        // delete dfa
        clean(tree_stack);
        delete dfa;
        // error out
        cerr << e.what() << endl;
        cerr << "ERROR" << endl;
    }
    return 0;
}
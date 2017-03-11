//
//  linker.cpp
//  OS lab1
//
//  Created by Haoran Wang on 2/20/17.
//  Copyright © 2017 Haoran Wang. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <utility>
#include <map>
#include <set>

#include "linker.hpp"

using std::cout;
using std::cerr;
using std::string;
using std::unitbuf;
using std::endl;
using std::vector;
using std::map;
using std::set;


// a function to get a token from the file text once at a time;
// "\n" "\t" " " are emitted;
// this function employs fstream.get() and fstream.peek() methods to process
string take_content(std::ifstream &instream,
                    unsigned &line_num,
                    unsigned &line_offset) {
    char character = instream.peek();
    if (character == -1) {
        return "EOF";
    }
    string content = string(1, character);
    // if the next character is one of "\n", "\t" and " "
    if ( content == "\n" || content == "\t" || content == " ") {
        instream.get();
        if (content == "\n") {
            if (instream.peek() != -1) {
                line_num++;
                line_offset = 0;
            }
        } else {
            line_offset++;
        }
        // use recursion to continue until next valid token
        return take_content(instream, line_num, line_offset);
    }
    
    // if the next character is not "\n" "\t" " ", take out the next token composed of a
    // series of consecutive characters
    else {
        instream.get();
        line_offset++;
        char next_character = instream.peek();
        if (next_character == -1) {
            line_offset++;
            return content;
        }
        string newcontent(1, next_character);
        // while the following character is not "\n", "\t", " "
        while ((newcontent!="\n") && (newcontent!="\t") && (newcontent!=" ")) {
            instream.get();
            line_offset++;
            content += newcontent;
            next_character = instream.peek();
            if (next_character == -1) {
                line_offset++;
                return content;
            }
            newcontent = string(1, next_character);
        }
        return content;
    }
}

// different parse errors to check during pass1
void parse_error(unsigned errcode,
                 const unsigned &line,
                 const unsigned offset) {
    
    static vector<string> errvec= {
        "NUM_EXPECTED", //errcode = 0
        "SYM_EXPECTED", //errcode = 1
        "ADDR_EXPECTED", //errcode = 2
        "SYM_TOO_LONG", //3
        "TO_MANY_DEF_IN_MODULE", //4
        "TO_MANY_USE_IN_MODULE", //5
        "TO_MANY_INSTR"}; //6
    cout << "Parse Error line " << line << " offset " << offset << ": " << errvec[errcode] << endl;
    exit(1);
}

// as indicated by function name
bool check_numexpected_error(const string &str,
                             unsigned &line,
                             unsigned line_offset,
                             bool moduleBegin) {
    
    if (str == "EOF") {
        if (!moduleBegin)
            parse_error(0, line - 1, line_offset + 1);
        else
            return true;
    }
    for (unsigned i = 0; i < str.size(); i++) {
        char character = str[i];
        if (character < '0' || character > '9') {
            parse_error(0, line, line_offset - i); // "NUM_EXPECTED"
        }
    }
    return false;
}

// as indicated by function name
void check_symbolexpected_error(const string &str,
                                unsigned &line,
                                unsigned line_offset) {
    
    if (str == "EOF") {
        parse_error(1, line, line_offset + 1);
    }
    char first = str[0];
    if (!((first >= 'a' && first <= 'z') || (first >= 'A' && first <= 'Z'))) {
        parse_error(1, line, line_offset + 1 - str.size());
    }
    
    if (str.size() > 16) {
        parse_error(3, line, line_offset + 1 - str.size());
    }
}

// as indicated by function name
void check_addressingexpected_error(const string &str,
                                    unsigned &line,
                                    unsigned line_offset) {
    if (str == "EOF") {
        parse_error(2, line, line_offset + 1);
    }
    set<string> valid = {"A", "R", "I", "E"};
    if (valid.find(str) == valid.end()) {
        parse_error(2, line, line_offset + 1 - str.size());
    }
}

/*
 * A function to parse the definition list;
 * Funtionalities differ for firstPass and secondPass
 */

bool parse_deflist(std::ifstream &instream,
                   const string &firstOrSecond,
                   unsigned &line,
                   unsigned &line_offset,
                   const unsigned &modu,
                   map<string, vector<string>> &symbol_table,
                   vector<string> &insertionOrder) {
    
    string content;
    content = take_content(instream, line, line_offset);
    if (check_numexpected_error(content, line, line_offset, 1)) {
        return true;
    }
    int defcount = std::stoi(content);
    if (defcount > 16) {
        parse_error(4, line, line_offset + 1 - std::to_string(defcount).size());
    }
    for (unsigned i = 0; i < defcount; i++) {
        string symbol = take_content(instream, line, line_offset);
        check_symbolexpected_error(symbol, line, line_offset);
        string rel_addr = take_content(instream, line, line_offset);
        check_numexpected_error(rel_addr, line, line_offset, 0);
        vector<string> information;
        if (firstOrSecond == "secondPass") continue;
        if (symbol_table.count(symbol) == 1) {
            (symbol_table.find(symbol)->second).push_back("Error: This variable is multiple times defined; first value used");
        } else {
            information.push_back(std::to_string(modu));
            information.push_back(rel_addr);
            symbol_table.insert({symbol, information});
            insertionOrder.push_back(symbol);
        }
    }
    return false;
}

/*
 * A function to parse the uselist;
 * Funtionalities differ for firstPass and secondPass
 */
void parse_uselist(std::ifstream &instream,
                   const string &firstOrSecond,
                   unsigned &line,
                   unsigned &line_offset,
                   vector<string> &uselist_token) {
    
    string content;
    content = take_content(instream, line, line_offset);
    if (firstOrSecond == "firstPass")
        check_numexpected_error(content, line, line_offset, 0);
    int usecount = std::stoi(content);
    if (firstOrSecond == "firstPass" && usecount > 16) {
        parse_error(5, line, line_offset + 1 - std::to_string(usecount).size());
    }
    for (unsigned i = 0; i < usecount; i++) {
        string symbol = take_content(instream, line, line_offset);
        if (firstOrSecond == "firstPass")
            check_symbolexpected_error(symbol, line, line_offset);
        else if (firstOrSecond == "secondPass") {
            uselist_token.push_back(symbol);
        }
    }
}

/* 
 * A function to parse the program text (instruction list);
 * Funtionalities differ for firstPass and secondPass
 */
void parse_instrlist(std::ifstream &instream,
                     const string &firstOrSecond,
                     unsigned &line,
                     unsigned &line_offset,
                     unsigned &modu,
                     unsigned &total_length,
                     map<string, vector<string>> &symbol_table,
                     map<unsigned, vector<unsigned>> &module_meta,
                     vector<string> &uselist_tokens,
                     set<string> &e_symbols,
                     set<string> &all_symbols,
                     string &memory_map) {
    
    string content;
    content = take_content(instream, line, line_offset);
    if (firstOrSecond == "firstPass")
        check_numexpected_error(content, line, line_offset, 0);
    unsigned instrcount = std::stoi(content);
    //update the length of the module and the base address for this module
    module_meta[modu] = vector<unsigned> {instrcount, total_length};
    //update the base address for next module
    total_length += instrcount;
    if (firstOrSecond == "firstPass" && total_length > 512) {
        parse_error(6, line, line_offset + 1 - std::to_string(instrcount).size());
    }
    
    for (unsigned i = 0; i < instrcount; i++) {
        string addressing = take_content(instream, line, line_offset);
        if (firstOrSecond == "firstPass")
            check_addressingexpected_error(addressing, line, line_offset);
        string address_val = take_content(instream, line, line_offset);
        if (firstOrSecond == "firstPass")
            check_numexpected_error(address_val, line, line_offset, 0);
        if (firstOrSecond == "secondPass") {
            unsigned line = module_meta[modu][1] + i;
            string line_str = line > 9 ? (line < 100 ? ("0" + std::to_string(line)) : std::to_string(line)) : ("00" + std::to_string(line));
            if (addressing == "I") {
                // rule 10
                if (address_val.size() > 4) {
                    memory_map += line_str + ": 9999 Error: Illegal immediate value; treated as 9999\n";
                } else {
                    while (address_val.size() < 4) {
                        address_val = "0" + address_val;
                    }
                    memory_map += line_str + ": " + address_val + "\n";
                }
            }
            else if (addressing == "A") {
                // rule 11
                if (address_val.size() > 4) {
                    memory_map += line_str + ": 9999 Error: Illegal opcode; treated as 9999\n";
                } else {
                    unsigned address = std::stoi(address_val.substr(1,3));
                    // rule 8
                    if (address > 512) {
                        address_val = std::to_string(std::stoi(address_val.substr(0,1))*1000);
                        memory_map += line_str + ": " + address_val +\
                                      " Error: Absolute address exceeds machine size; zero used\n";
                    } else {
                        memory_map += line_str + ": " + address_val + "\n";
                    }
                }
            }
            else if (addressing == "R") {
                if (address_val.size() > 4) {
                    memory_map += line_str + ": 9999 Error: Illegal opcode; treated as 9999\n";
                } else {
                    unsigned rel_address = std::stoi(address_val)%1000;
                    string err_info = "\n";
                    // rule 9
                    if (rel_address >= module_meta[modu][0]) {
                        rel_address = 0;
                        err_info = " Error: Relative address exceeds module size; zero used\n";
                        
                    }
                    unsigned address = rel_address + module_meta[modu][1] + std::stoi(address_val)/1000 * 1000;
                    memory_map += line_str + ": " + std::to_string(address) + err_info;
                }
            }
            else if (addressing == "E") {
                // rule 11
                if (address_val.size() > 4) {
                    memory_map += line_str + ": 9999 Error: Illegal opcode; treated as 9999\n";
                } else {
                    unsigned position = std::stoi(address_val)%1000;
                    //rule 6
                    if (position >= uselist_tokens.size()) {
                        memory_map += line_str + ": " + address_val +\
                                     " Error: External address exceeds length of uselist; treated as immediate\n";
                    } else {
                        string symbol = uselist_tokens[position];
                        // rule 3
                        if (symbol_table.count(symbol) != 1) {
                            memory_map += line_str + ": " + std::to_string(std::stoi(address_val.substr(0,1))*1000) +\
                                          " Error: " + symbol + " is not defined; zero used\n";
                        } else {
                            unsigned abs_address = std::stoi(symbol_table[symbol][1]);
                            string address = std::to_string(std::stoi(address_val)/1000 * 1000 + abs_address);
                            memory_map += line_str + ": " + address + "\n";
                        }
                        e_symbols.insert(symbol);
                        all_symbols.insert(symbol);
                    }
                }
            }
        }
    }
}


/*
 * This is a function that parses all modules in a file using the functions defined above
 */

void parse_modules(std::ifstream &instream, const string &firstOrSecond) {
    unsigned total_length = 0;
    unsigned module = 0;
    unsigned line = 1;
    unsigned line_offset = 0;
    
    // a vector to maintain the insertion order of symbols, to be used
    // for printing out the symbol table
    static vector<string> insertionOrderSym;
    // symbol table metadata, which is a map to store the symbol defined as key,
    // and its address and relevant error information, if any, as values
    static map<string, vector<string>> symbol_table;
    // module metadata, which is a map to store the module number as key,
    // and the length of module and its base address as values
    static map<unsigned, vector<unsigned>> module_meta;
    // memory output for second pass output
    string memory_map = "Memory Map\n";
    // use list token for each module for pass2
    vector<string> uselist_tokens;
    // a set to store the E-type symbols for each module during pass2
    set<string> e_symbols;
    // a set to store all symbols in the instruction lists for all modules in a file
    set<string> all_symbols;
    
    // if we do the first pass to parse the input
    if (firstOrSecond == "firstPass") {
        while (instream.peek() != -1) {
            // each run inside the while loop corresponds to a new module
            module++;
            if (!parse_deflist(instream, "firstPass", line, line_offset, module, symbol_table, insertionOrderSym)) {
                parse_uselist(instream, "firstPass", line, line_offset, uselist_tokens);
                parse_instrlist(instream, "firstPass", line, line_offset, module, total_length, symbol_table, module_meta, uselist_tokens, e_symbols, all_symbols, memory_map);
            }
            // check the address size of symbols after each module pass;
            // print warning if the address exceeds the module length and change
            // the corresponding address associated with the symbol
            for (auto &ele : symbol_table) {
                const vector<string> information = ele.second;
                const unsigned modu = std::stoi(information[0]);
                if (modu == module && (std::stoi(information[1]) >= module_meta[module][0])) {
                    // not the insertion order
                    cout << "Warning: Module " << module << ": " << ele.first << " too big "
                         << std::stoi(information[1]) << " (max=" << module_meta[modu][0]-1
                         << ")" << " assume zero relative" << endl;
                    ele.second[1] = "0";
                }
            }
        }
        string output("Symbol Table\n");
        // loop through the symbols to print out the symbol table
        for (const auto &symbol : insertionOrderSym) {
            output += symbol;
            vector<string> &information = symbol_table[symbol];
            for (unsigned i = 1; i < information.size(); i++) {
                if (i == 1) {
                    unsigned module_associated = std::stoi(information[0]);
                    string addr = std::to_string(std::stoi(information[i]) + module_meta[module_associated][1]);
                    information[1] = addr;
                    output += ("=" + addr);
                }
                else
                    output += (" " + information[i]);
            }
            output += "\n";
        }
        cout << output << endl;
    // if we do the second pass to parse the input
    } else if (firstOrSecond == "secondPass") {
        module = 0;
        while (instream.peek() != -1) {
            // each run inside the while loop corresponds to a new module
            module++;
            if (!parse_deflist(instream, "secondPass", line, line_offset, module, symbol_table, insertionOrderSym)) {
                parse_uselist(instream, "secondPass", line, line_offset, uselist_tokens);
                parse_instrlist(instream, "secondPass", line, line_offset, module, total_length, symbol_table, module_meta, uselist_tokens, e_symbols, all_symbols, memory_map);
            }
            // print out the result for each module in pass2
            cout << memory_map;
            for (const auto &token : uselist_tokens) {
                if (e_symbols.find(token) == e_symbols.end()) {
                    cout << "Warning: Module " << module << ": " << token << " appeared in the uselist but was not actually used" << endl;
                }
            }
            e_symbols.clear();
            uselist_tokens.clear();
            memory_map = "";
        }
        cout << endl;
        for (const auto &symbol : insertionOrderSym) {
            if (all_symbols.find(symbol) == all_symbols.end()) {
                cout << "Warning: Module " << symbol_table[symbol][0] << ": " << symbol << " was defined but never used" << endl;
            }
        }
    } else{
        std::cerr << "The second argument to parse_modules should be either firstPass or Second Pass" << endl;
        exit(1);
    }
}


int main(int argc, const char * argv[]) {
    string file = argv[1];
    std::ifstream instream(file);
    if (instream) {
        parse_modules(instream, "firstPass");
        instream.close();
        std::ifstream instream(file);
        parse_modules(instream, "secondPass");
        return 0;
    }
    cerr << "The file is not properly opened." << endl;
    return 1;
}

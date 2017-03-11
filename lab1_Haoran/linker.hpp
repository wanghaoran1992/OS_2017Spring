//
//  linker.hpp
//  OS
//
//  Created by Haoran Wang on 2/20/17.
//  Copyright © 2017 Haoran Wang. All rights reserved.
//

#ifndef linker_hpp
#define linker_hpp

#include <stdio.h>

using std::string;
using std::vector;
using std::map;
using std::set;

string take_content(std::ifstream &instream,
                    unsigned &line_num,
                    unsigned &line_offset);

void parse_error(unsigned errcode,
                 const unsigned &line,
                 const unsigned offset);

bool check_numexpected_error(const string &str,
                             unsigned &line,
                             unsigned line_offset,
                             bool moduleBegin);

void check_symbolexpected_error(const string &str,
                                unsigned &line,
                                unsigned line_offset);

void check_addressingexpected_error(const string &str,
                                    unsigned &line,
                                    unsigned line_offset);

bool parse_deflist(std::ifstream &instream,
                   const string &firstOrSecond,
                   unsigned &line,
                   unsigned &line_offset,
                   const unsigned &modu,
                   map<string, vector<string>> &symbol_table,
                   vector<string> &insertionOrder);

void parse_uselist(std::ifstream &instream,
                   const string &firstOrSecond,
                   unsigned &line,
                   unsigned &line_offset,
                   vector<string> &uselist_token);

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
                     string &memory_map);

void parse_modules(std::ifstream &instream, const string &firstOrSecond);


#endif /* linker_hpp */

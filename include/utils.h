#ifndef UTILS_H
#define UTILS_H

#include <fstream>
#include <iostream>
#include <map>
#include <vector>

#define KILO 1000
#define MB150 (1L * KILO * KILO * 150)

using namespace std;

class utils {
  public:
    static void write_file(const string, const char*, const long);
    static long split_file(const string, vector<const char*>**);
    static map<string, string> quick_parse(const string&);
    static void ls(const char*, const string);
};

#endif // UTILS_H
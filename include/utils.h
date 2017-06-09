#ifndef UTILS_H
#define UTILS_H

#include <fstream>
#include <iostream>
#include <map>
#include <vector>

#define KILO 1000
#define MEGS 150
#define MB150 (1L * KILO * KILO * MEGS)

using namespace std;

class utils {
  public:
    static void split_file(const string, const vector<const char*>&);
    static map<string, string> quick_parse(const string&);
    static void ls(const char*, const string);
};

#endif // UTILS_H

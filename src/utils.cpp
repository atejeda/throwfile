#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

#include <dirent.h>
#include <errno.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

using namespace std;

void utils::split_file(const string path, const vector<const char*>& holder) {
    const long chunk_size = MB150;
    // dd if=/dev/zero of=~/1GB.dat bs=100M count=10
    // dd if=/dev/zero of=~/1GB.dat bs=262144000 count=4

    ifstream file;
    file.open(path, ios::in | ios::binary | ios::ate);
    if (!file.is_open() || !file.good()) {
        cout << "there's some error opening file..." << endl;
        return;
    }

    long size = file.tellg();
    file.seekg(0, ios::beg);

    int pieces = size / chunk_size;
    int remain = size % chunk_size;

    long lower, upper, block_size, sum = 0;

    for (int i = 0; i <= pieces; i++) {
        lower = i * chunk_size;
        upper = ((i + 1) * chunk_size) - 1;
        block_size = i < pieces ? chunk_size : remain;
        sum += block_size;

        char* block_mem = new char[block_size];
        file.read(block_mem, block_size);
        // holder.push_back(nullptr);
    }

    file.close();
}

map<string, string> utils::quick_parse(const string& json) {
    // - I assume that the json data is valid
    // - This is a very simple parser
    // - Doesn't consider null, true and false as a value,
    //   see https://i.stack.imgur.com/NYWuA.gif
    // - It works with just "value":"key" json format
    // - It works just at level 0 of the json tree
    // - It is intended to parse the dropbox responses

    string current;
    string last;
    bool start = false;
    bool iskey = true;

    map<string, string> data;

    for (int i = 0; i < json.size(); i++) {
        char c = json[i];

        if (isspace(c) || c == '{' || c == ',' || c == ':' || c == '}')
            continue;

        if (c == '"' && current[current.size() - 1] != '\\') {
            start = !start;

            if (!start) {
                if (!iskey)
                    data[last] = current;
                iskey = !iskey;
                last = current;
                current = "";
            }

            continue;
        }

        current += c;
    }

    return data;
}

void utils::ls(const char* path, const string parent = "") {
    DIR* directory;
    if ((directory = opendir(path)) == NULL) {
        cout << "throwfile: cannot access " << path << ": " << strerror(errno)
             << endl;
        return;
    }

    struct dirent* entry;
    struct stat entry_stat;
    string name;
    string fullpath;

    while ((entry = readdir(directory))) {
        name = entry->d_name;
        fullpath = string(path) + "/" + name;

        stat(fullpath.c_str(), &entry_stat);

        if (S_ISLNK(entry_stat.st_mode)) {
            continue; // symlinks not supported
        } else if (S_ISREG(entry_stat.st_mode)) {
            cout << parent << "/" << name << endl;
        } else if (S_ISDIR(entry_stat.st_mode)) {
            if (name == ".." || name == ".")
                continue;
            ls(fullpath.c_str(), parent + "/" + name);
        }
    }

    closedir(directory);
}

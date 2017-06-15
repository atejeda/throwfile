#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

#include <dirent.h>
#include <errno.h>
#include <fstream>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

using namespace std;

void utils::write_file(const string file_name, const char* buffer,
                       const long size) {
    ofstream file("/home/atejeda/data0." + file_name, ios::out | ios::binary);
    if (file.is_open() && file.good()) {
        file.write(buffer, size);
        file.close();
    }
}

long utils::split_file(const string path, vector<const char*>** holder) {
    (*holder) = new vector<const char*>();
    auto holder_v = (*holder);

    ifstream file;
    file.open(path, ios::in | ios::binary | ios::ate);
    if (!file.is_open() || !file.good()) {
        cout << "there's some error opening file..." << endl;
        return -1;
    }

    // dd if=/dev/zero of=~/1GB.dat bs=100M count=10
    // dd if=/dev/zero of=~/1GB.dat bs=262144000 count=4

    long size = file.tellg();
    file.seekg(0, ios::beg);

    const long chunk_size = MB150;
    const long pieces = size / chunk_size;
    const long remain = size % chunk_size;

    holder_v->reserve(pieces + 1);

    long lower, upper, block_size, sum = 0;

    for (int i = 0; i <= pieces; i++) {
        lower = i * chunk_size;
        upper = ((i + 1) * chunk_size) - 1;
        block_size = i < pieces ? chunk_size : remain;
        sum += block_size;
        char* block_mem = new char[block_size];
        file.read(block_mem, block_size);
        holder_v->push_back(block_mem);
    }

    file.close();

    return remain;
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

        if ((isspace(c) || c == '{' || c == ',' || c == ':' || c == '}') &&
            !start)
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

void utils::ls(const string path, vector<throwfile_path_t>* files_path,
               const string parent) {
    // return a completion here
    //  check for errors, realpath

    DIR* directory;

    if ((directory = opendir(path.c_str())) == NULL) {
        cout << "throwfile: cannot access ";
        cout << path << ": " << strerror(errno) << endl;
        return;
    }

    struct dirent* entry;
    struct stat entry_stat;

    string file_name;

    string root = static_cast<string>(basename(path.c_str()));
    string system_path;
    string dropbox_path;

    // throwfile_path_t

    while ((entry = readdir(directory))) {
        file_name = entry->d_name;
        system_path = string(path + '/' + file_name);
        dropbox_path = string(parent + '/' + root + '/' + file_name);

        stat(system_path.c_str(), &entry_stat);

        if (S_ISLNK(entry_stat.st_mode)) {
            continue;
        } else if (S_ISREG(entry_stat.st_mode)) {
            float file_size = entry_stat.st_size;
            files_path->push_back(throwfile_path_t{
                system_path, dropbox_path, static_cast<long>(file_size)});
        } else if (S_ISDIR(entry_stat.st_mode)) {
            if (file_name == ".." || file_name == ".")
                continue;
            ls(system_path, files_path, parent + '/' + root);
        }
    }

    closedir(directory);
//    delete resolved_path;
}

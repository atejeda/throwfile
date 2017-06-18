#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
#include <iomanip>
#include <sstream>

#include <dirent.h>
#include <errno.h>
#include <fstream>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

using namespace std;

// TODO:
// - return completion (?)
// - add destination
// - add realpath (and check errors)
void utils::write_file(const string file_name, const char* buffer, const long size) {
    ofstream file("/home/atejeda/data0." + file_name, ios::out | ios::binary);
    if (file.is_open() && file.good()) {
        file.write(buffer, size);
        file.close();
    }
}

// TODO:
// - return completion (?)
// dd if=/dev/zero of=~/1GB.dat bs=100M count=10
// dd if=/dev/zero of=~/1GB.dat bs=262144000 count=4
// filesize
// chunk size
long utils::split_file(const string path, const long size, const long chunk_size, vector<const char*>** holder) {
    (*holder) = new vector<const char*>();
    auto holder_v = (*holder);

    ifstream file;
    file.open(path, ios::in | ios::binary);
    if (!file.is_open() || !file.good()) {
        cout << "there's some error opening file..." << endl;
        return -1;
    }

    //long size = file.tellg();
    //file.seekg(0, ios::beg);

    const long pieces = size / chunk_size;
    const long remain = size % chunk_size;

    holder_v->reserve(pieces + 1);

    for (int i = 0; i <= pieces; i++) {
        long block_size = i < pieces ? chunk_size : remain;
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

        if ((isspace(c) || c == '{' || c == ',' || c == ':' || c == '}') && !start)
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

// TODO:
// - return completion
// - check for errors, realpath
void utils::ls(const string path, vector<throwfile_path_t>* files_path, const string file_parent) {

    char resolved_path[PATH_MAX + 1];
    realpath(path.c_str(), resolved_path);

    DIR* directory;
    if ((directory = opendir(resolved_path)) == NULL) {
        cout << "throwfile: cannot access ";
        cout << path  << " -> " << resolved_path << ": " << strerror(errno) << endl;
        return;
    }

    struct dirent* entry;
    struct stat entry_stat;

    string local_path;
    string remote_path;

    string file_name;
    string file_root = static_cast<string>(basename(resolved_path));

    while ((entry = readdir(directory))) {
        file_name = entry->d_name;

        local_path = static_cast<string>(resolved_path) + '/' + file_name;
        remote_path = file_parent + '/' + file_root + '/' + file_name;

        stat(local_path.c_str(), &entry_stat);

        if (S_ISLNK(entry_stat.st_mode)) {
            continue;
        } else if (S_ISREG(entry_stat.st_mode)) {
            float file_size = entry_stat.st_size;
            files_path->push_back(throwfile_path_t{local_path, remote_path, "B", static_cast<long>(file_size), 0l, 1l});
        } else if (S_ISDIR(entry_stat.st_mode)) {
            if (file_name == ".." || file_name == ".")
                continue;
            ls(local_path, files_path, file_parent + '/' + file_root);
        }
    }

    closedir(directory);
}


string utils::get_size_unit(const long size) {
    int file_size_temporal;
    double file_size;
    string file_size_unit;
    bool file_size_precision = false;

    if ((file_size_temporal = (file_size = size / 1000000000.0)) > 0) {
        file_size_precision = size % 1000000000 > 0;
        file_size_unit = "GB";
    } else if ((file_size_temporal = (file_size = size / 1000000.0)) > 0) {
        file_size_precision = size % 1000000 > 0;
        file_size_unit = "MB";
    } else if ((file_size_temporal = (file_size = size / 1000.0)) > 0) {
        file_size = file_size_temporal;
        file_size_unit = "KB";
    } else {
        file_size = size;
        file_size_unit = "B";
    }

    stringstream output;
    if (file_size_precision)
        output << std::fixed << std::setprecision(2);
    output << file_size;
    output << " ";
    output << file_size_unit;

    return output.str();
}

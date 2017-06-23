/*
 * A mini c++ application to send files to dropbox. 
 * -----------------------------------------------------------------------------
 *
 *            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *                    Version 2, December 2004
 *
 * Copyright (C) 2017 Alexis Tejeda
 *
 * Everyone is permitted to copy and distribute verbatim or modified
 * copies of this license document, and changing it is allowed as long
 * as the name is changed.
 *
 *            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
 *
 *  0. You just DO WHAT THE FUCK YOU WANT TO.
 *
 * -----------------------------------------------------------------------------
 * 
 * Thirdparty:
 * 
 * This application uses licurl to do all the post and other things related to
 * networking, interested in libcurl liecense? go ahead and take a look
 * https://curl.haxx.se/docs/copyright.html
 * 
 * Dropbox policies, take a look to the developers dropbox site 
 * https://www.dropbox.com/developers/reference/tos
 * 
 * -----------------------------------------------------------------------------
 *
 * Disclaimer:
 * 
 * This is not an official dropbox application, it only uses the dropbox API,
 * OAuth2 workflow provided by dropbox is used to authenticate the application
 * using a token to do all the dropbox interaction later.
 * 
 */

/*
TODO:
- file with regex expressions (v2)
- just big files   (v2)
- just small files (v2)
- upload files
- add total time spent as sum of all uploads
- summary of how much will be uplodad (data size)
- register errors
*/

// -------------------------------------------------------------------------- //

/* compiler and app define's */

// gcc.gnu.org/onlinedocs/gcc-5.2.0/libstdc++/manual/manual/using_dual_abi.html
// main.cpp:(.text+0x28): undefined reference to 
// `std::__cxx11::basic_string<char, std::char_traits<char>, 
// std::allocator<char> >::basic_string()'
// .. libcurl string compatibility
#define _GLIBCXX_USE_CXX11_ABI 0

/* linux includes */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* c includes */
#include <unistd.h>

/* c includes (c++) */
#include <cstring>
#include <cstdlib>
#include <ctime>

/* c++ includes */
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <vector>
#include <map>
#include <algorithm>

/* other includes */
#include <curl/curl.h>

/* defines */

#define EP_OAUTH2 \
    "https://api.dropboxapi.com/oauth2/token"

#define EP_ACCOUNT \
    "https://api.dropboxapi.com/1/account/info"

#define EP_UPLOAD \
    "https://content.dropboxapi.com/2/files/upload"

#define EP_UPLOAD_SESSION_START \
    "https://content.dropboxapi.com/2/files/upload_session/start"

#define EP_UPLOAD_SESSION_APPEND \
    "https://content.dropboxapi.com/2/files/upload_session/append_v2"

#define EP_UPLOAD_SESSION_FINISH \
    "https://content.dropboxapi.com/2/files/upload_session/finish"

#define CKSIZE (1024 * 1024 * 150) // K as 1024, 150 MB

/* namespaces */
using namespace std;

/* structs, unions and enums */

struct path_t {
    string lo; // local
    string re; // remote
    size_t size;
};

struct progress_data_t {
    double speed_sum;
    int speed_count;
    int chunk_total;
    int chunk_current;
    bool done;
    CURL* handler;
};

enum session_mode { 
    single, 
    session_start, 
    session_append, 
    session_finish 
};

/* typedefs */

// typedef struct path_t path_t;
// typedef struct progress_data_t progress_data_t;

/* application variables */

// application credentials
const static string app_key = "y5dfulqv9ayvjty";
const static string app_secret = "b0ucqd3w9a9tqja";

// handles the responses from the handler
static stringstream handlerv_response;

// progress data
progress_data_t progress_data;

/* functions declarations */

/* handlers */

size_t handlerf_response(void* buffer, size_t size, size_t nmemb, void* userp) {
    size_t realsize;

    if ((realsize = size * nmemb)) {
        char* read_buffer = new char[realsize];
        strncpy(read_buffer, (char*)buffer, realsize);
        handlerv_response << read_buffer;
        delete read_buffer;
    }

    return realsize;
}

/* utils */

map<string, string> quick_parse(const string& json) {
    string current;
    string last;
    bool start = false;
    bool iskey = true;

    map<string, string> data;

    for (int i = 0; i < json.size(); i++) {
        char c = json[i];

        if ((isspace(c) || c == '{' || c == ',' || c == ':' || c == '}') 
            && !start)
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

string real_path(string path) {
    string home{getpwuid(getuid())->pw_dir};
    if (path[0] == '~') path = home + path.substr(1);
    char resolved[PATH_MAX + 1];
    realpath(path.c_str(), resolved);
    // check for errors
    return static_cast<string>(resolved);
}

void ls(const string path, vector<path_t>& files, const string parent) {
    const char* real_path = path.c_str();

    DIR* dir;
    if ((dir = opendir(real_path)) == NULL) {
        //strerror(errno);
    }

    struct dirent* entry;
    struct stat entry_stat;

    string lo; // local
    string re; // remote

    string name;
    string root = static_cast<string>(basename(real_path));

    while ((entry = readdir(dir))) {
        name = entry->d_name;

        lo = static_cast<string>(real_path) + '/' + name;
        re = parent + '/' + root + '/' + name;

        stat(lo.c_str(), &entry_stat);

        if (S_ISLNK(entry_stat.st_mode)) {
            continue;
        } else if (S_ISREG(entry_stat.st_mode)) {
            size_t size = entry_stat.st_size;
            files.push_back(path_t{lo, re, static_cast<size_t>(size)});
        } else if (S_ISDIR(entry_stat.st_mode)) {
            if (name == ".." || name == ".")
                continue;
            ls(lo, files, parent + '/' + root);
        }
    }

    closedir(dir);
}

long file_split_sizes(const size_t size, const size_t csize, long& pieces) {
    pieces = size / csize;
    return size % csize;
}

long file_split(const string path, const size_t size, const size_t csize, 
    vector<const char*>& holder) {

    ifstream file;
    file.open(path, ios::in | ios::binary); 
    if (!file.is_open() || !file.good()) {
        // error: cout << "there's some error opening file..." << endl;
        return -1;
    }

    // ios::ate
    //long size = file.tellg();
    //file.seekg(0, ios::beg);

    long pieces;
    long remain;

    remain = file_split_sizes(size, csize, pieces);

    holder.reserve(pieces + 1);

    for (int i = 0; i <= pieces; i++) {
        long block_size = i < pieces ? csize : remain;
        char* block = new char[block_size];
        file.read(block, block_size);
        holder.push_back(block);
    }

    file.close();

    return remain;
}

string size_unit(const size_t size) {
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
    if (file_size_precision) output << std::fixed << std::setprecision(2);
    output << file_size;
    output << " ";
    output << file_size_unit;

    return output.str();
}

string time_unit(const size_t size) {
    int time_len_temporal;
    double time_len;
    string time_len_unit;
    bool file_size_precision = false;

    if ((time_len_temporal = (time_len = size / 3600.0)) > 0) {
        file_size_precision = size % 3600 > 0;
        time_len_unit = "hr";
    } else if ((time_len_temporal = (time_len = size / 60.0)) > 0) {
        file_size_precision = size % 60 > 0;
        time_len_unit = "m";
    } else {
        time_len = size;
        time_len_unit = "s";
    }

    stringstream output;
    if (file_size_precision) output << std::fixed << std::setprecision(2);
    output << time_len;
    output << time_len_unit;

    return output.str();
}

/* header param setup */

void session_param_start(const bool close, string& str) {
    const string q = "\"";
    stringstream out;
    
    out << "{"; // start

    // close
    out << q << "close" << q << ":" << std::boolalpha << close;

    out << "}"; // ends

    str = out.str();
}

void session_param_append(const string session, const size_t offset,
                          const bool close, string& str) {
    const string q = "\"";
    stringstream out;
    
    out << "{"; // start
    
    // cursor
    out << q << "cursor" << q << ":";
    out << "{";
    out << q << "session_id" << q << ":" << q << session << q;
    out << ",";
    out << q << "offset" << q << ":" << offset;
    out << "}";

    out << ",";

    // close
    out << q << "close" << q << ":" << std::boolalpha << close;

    out << "}"; // ends

    str = out.str();
}

void session_param_finish(const string session, const size_t offset, 
                    const string path, const string mode, const bool autorename, 
                    const bool mute, string& str) {
    const string q = "\"";
    stringstream out;
    
    out << "{"; // start
    
    // cursor
    out << q << "cursor" << q << ":";
    out << "{";
    out << q << "session_id" << q << ":" << q << session << q;
    out << ",";
    out << q << "offset"     << q << ":"      << offset;
    out << "}";

    out << ",";

    // commit
    out << q << "commit" << q << ":";
    out << "{";
    out << q << "path"       << q << ":" << q << path << q;
    out << ",";
    out << q << "mode"       << q << ":" << q << mode << q;
    out << ",";
    out << q << "autorename" << q << ":" << std::boolalpha << autorename;
    out << ",";
    out << q << "mute"       << q << ":" << std::boolalpha << mute;
    out << "}";

    out << "}"; // ends

    str = out.str();
}

/* oauth2 */

bool oauth2_token_file_get(string& token) {
    // check errors

    bool success = false;

    string home{getpwuid(getuid())->pw_dir};
    string path{home + "/.throwfile"};

    ifstream file(path);
    if ((success = (file.is_open() && file.good()))) {
        getline(file, token);
        success = !token.empty();
        file.close();
    }

    return success;
}

bool oauth2_token_file_set(const string token) {
    // check errors

    bool success = false;

    string home{getpwuid(getuid())->pw_dir};
    string path{home + "/.throwfile"};

    ofstream file(path);
    if ((success = (file.is_open() && file.good()))) {
        file << token;
        file.close();
    }

    return success;
}

bool oauth2_token_remote_get(string& token, const string app_auth) {
    CURL* handler;

    if (!(handler = curl_easy_init())) {
        // print err?
        return false;
    }

    // headers
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, 
        "Content-Type: application/x-www-form-urlencoded;");

    // post data
    string post = "code=" + app_auth;
    post += string("&grant_type=authorization_code");
    post += "&client_id=" + app_key + "&client_secret=" + app_secret;

    curl_easy_setopt(handler, CURLOPT_URL, EP_OAUTH2);
    curl_easy_setopt(handler, CURLOPT_WRITEFUNCTION, handlerf_response);
    curl_easy_setopt(handler, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(handler, CURLOPT_POSTFIELDS, post.c_str());

    CURLcode res_curl;
    string res_data;

    // perform the request
    
    handlerv_response.clear();

    if ((res_curl =curl_easy_perform(handler)) != CURLE_OK) {
        // request problem, print err ?
        return false;
    } else if ((res_data = handlerv_response.str()).length() <= 0) {
        // there is no data, return, print err?
        // "unknown error (1)"
        return false;
    }

    // parse the data
    map<string, string> dict = quick_parse(res_data);

    bool istoken = dict.find("access_token") != dict.end();
    bool iserror = dict.find("error_description") != dict.end();

    if (istoken) {
        token = dict["access_token"];
    } else if (iserror) {
        //dict["error_description"];
        return false;
    } else {
        //"unknown error (2)"
        return false;
    }

    // clean up the handler and return
    curl_easy_cleanup(handler);

    return false; 
}

bool oauth_token_validate(const string token, string& res) {
    CURL* handler;

    if (!(handler = curl_easy_init())) {
        // print err?
        return false;
    }

    // headers
    struct curl_slist* headers = nullptr;
    string bearer = "Authorization: Bearer " + token;
    headers = curl_slist_append(headers, bearer.c_str());

    string post;

    curl_easy_setopt(handler, CURLOPT_URL, EP_ACCOUNT);
    curl_easy_setopt(handler, CURLOPT_WRITEFUNCTION, handlerf_response);
    curl_easy_setopt(handler, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(handler, CURLOPT_POSTFIELDS, post.c_str());

    // perform the request

    handlerv_response.str(string());

    CURLcode res_curl;
    string res_data;    

    if ((res_curl = curl_easy_perform(handler)) != CURLE_OK) {
        // request problem, print err ?
        return false;
    } else if ((res_data = handlerv_response.str()).length() <= 0) {
        // there is no data, return, print err?
        // "unknown error (1)"
        return false;
    }

    // parse the data
    map<string, string> dict = quick_parse(res_data);

    bool istoken = dict.find("display_name") != dict.end();
    bool iserror = dict.find("error_description") != dict.end();

    if (istoken) {
        res = dict["display_name"];
    } else if (iserror) {
        //dict["error_description"];
        return false;
    } else {
        //"unknown error (2)"
        return false;
    }

    // clean up the handler and return
    curl_easy_cleanup(handler);

    return true; 
}

bool oauth2_token_get(string& token) {
    if (oauth2_token_file_get(token))
        return true;

    // ask for the auth code, should be done outside this function

    string url = "https://www.dropbox.com/oauth2/authorize";
    url += "?client_id=" + app_key;
    url += "&response_type=code";

    string auth_input;

    cout << "Token configuration is empty or doesn't exists:" << endl;
    cout << "To authorize this application,";
    cout << "copy this address below in your browser, ";
    cout << "then copy the provided auth code here" << endl;
    cout << "\e[1m" << url << "\e[0m" << endl;
    cout << "Paste the auth code here : ";
    cin >> auth_input;

    if (auth_input.empty())
        return false; // register error

    // perform the request

    // return on error
    if (!oauth2_token_remote_get(token, auth_input)) {
        return false;
    }

    // persist the token for the future, and return the completion
    return oauth2_token_file_set(token);
}

/* upload */

int progress(void *p, double dlt, double dlu, double upt, double upn) {
    progress_data_t* progress_data = static_cast<progress_data_t*>(p);
    int percent = (upn/upt) * 100;

    if (percent < 0 || percent > 100 || progress_data->done) return 0;
    
    // upload info
    double time = 0;
    double speed = 0;

    CURL* handler = progress_data->handler;

    curl_easy_getinfo(handler, CURLINFO_TOTAL_TIME, &time);
    curl_easy_getinfo(handler, CURLINFO_SPEED_UPLOAD, &speed);

    cerr << "                              \r";
    cerr << "|progress   : " << progress_data->chunk_current;
    cerr << " of " << progress_data->chunk_total << " " <<  percent << "%";
    
    if (percent < 100) {
        cerr << " at " << size_unit(speed) << "/sec";
        cerr << "\r";
    } else {
        double avgs = progress_data->speed_sum / progress_data->speed_count;
        cout << " (" << size_unit(avgs) << "/sec avg)" << endl;
    }

    cout << std::flush;
    cerr << std::flush;

    // update struct
    progress_data->done = percent >= 100;
    progress_data->speed_sum += speed;
    progress_data->speed_count++; 
    
    return 0;
}

bool upload(const string& token, const path_t& file, long& took) {
    CURL* handler;

    if (!(handler = curl_easy_init())) {
        // print err?
        return false;
    }

    // headers
    struct curl_slist* headers = nullptr;
    
    string bearer = "Authorization: Bearer " + token;
    headers = curl_slist_append(headers, bearer.c_str());

    string octet = "Content-Type: application/octet-stream";
    headers = curl_slist_append(headers, octet.c_str());

    string api = "Dropbox-API-Arg: ";
    api += "{ \"path\" : \"" + file.re + "\"";
    api += + ", \"mode\" : \"overwrite\" }";
    headers = curl_slist_append(headers, api.c_str());

    // file setup
    vector<const char*> blocks;
    size_t remaining = file_split(file.lo, file.size, CKSIZE, blocks);

    const char* block = blocks[0]; // check this with valgrind, memleaks

    curl_easy_setopt(handler, CURLOPT_URL, EP_UPLOAD);
    curl_easy_setopt(handler, CURLOPT_WRITEFUNCTION, handlerf_response);
    curl_easy_setopt(handler, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(handler, CURLOPT_POSTFIELDS, block);

    // progress

    progress_data.speed_sum = 0;
    progress_data.speed_count = 0;
    progress_data.done = false;
    progress_data.handler = handler;

    curl_easy_setopt(handler, CURLOPT_PROGRESSFUNCTION, progress);
    curl_easy_setopt(handler, CURLOPT_PROGRESSDATA, &progress_data);
    curl_easy_setopt(handler, CURLOPT_NOPROGRESS, 0L);

    // perform the request

    handlerv_response.str(string());

    CURLcode res_curl;
    string res_data = "nodata";

    time_t time_start = time(nullptr);
    if ((res_curl = curl_easy_perform(handler)) != CURLE_OK) {
        // request problem, print err ?
        took = difftime(time(nullptr), time_start);
        return false;
    } else if ((res_data = handlerv_response.str()).length() <= 0) {
        // there is no data, return, print err?
        // "unknown error (1)"
        took = difftime(time(nullptr), time_start);
        return false;
    }
    took = difftime(time(nullptr), time_start);

    // parse the data
    map<string, string> dict = quick_parse(res_data);

    bool istoken = dict.find("display_name") != dict.end();
    bool iserror = dict.find("error_description") != dict.end();

    if (istoken) {
        // res = dict["display_name"];
    } else if (iserror) {
        //dict["error_description"];
        return false;
    } else {
        //"unknown error (2)"
        return false;
    }

    // clean up the handler and return
    curl_easy_cleanup(handler);

    return false; 
}

/* upload session */

bool upload_session(const string& token, string& session, const char* block, 
                    const size_t offset, long& took, const path_t& file, 
                    const int mode) {
    string endpoint;
    string api = "Dropbox-API-Arg: ";
    string par;
    
    if (mode == session_start) {
        // session start
        session_param_start(false, par);
        api += par;
        endpoint = EP_UPLOAD_SESSION_START;
    } else if (mode == session_append) {
        // session append
        session_param_append(session, offset, false, par);
        api += par;
        endpoint = EP_UPLOAD_SESSION_APPEND;
    } else if (mode == session_finish) {
        // session finish
        const string dmode = "overwrite"; // add
        session_param_finish(session, offset, file.re, dmode, true, false, par);
        api += par;
        endpoint = EP_UPLOAD_SESSION_FINISH;
    } else {
        // print error
        return false;
    }

    CURL* handler;

    if (!(handler = curl_easy_init())) {
        // print err?
        return false;
    }

    // headers
    struct curl_slist* headers = nullptr;
    
    string bearer = "Authorization: Bearer " + token;
    headers = curl_slist_append(headers, bearer.c_str());

    string octet = "Content-Type: application/octet-stream";
    headers = curl_slist_append(headers, octet.c_str());
    headers = curl_slist_append(headers, api.c_str());
    
    curl_easy_setopt(handler, CURLOPT_URL, endpoint.c_str());
    curl_easy_setopt(handler, CURLOPT_WRITEFUNCTION, handlerf_response);
    curl_easy_setopt(handler, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(handler, CURLOPT_CUSTOMREQUEST, "POST");

    if (block)
        curl_easy_setopt(handler, CURLOPT_POSTFIELDS, block);

    // progress

    progress_data.speed_sum = 0;
    progress_data.speed_count = 0;
    progress_data.done = false;
    progress_data.handler = handler;

    curl_easy_setopt(handler, CURLOPT_PROGRESSFUNCTION, progress);
    curl_easy_setopt(handler, CURLOPT_PROGRESSDATA, &progress_data);
    curl_easy_setopt(handler, CURLOPT_NOPROGRESS, 0L);

    // perform the request

    handlerv_response.str(string());

    CURLcode res_curl;
    string res_data = "nodata";

    time_t time_start = time(nullptr);
    if ((res_curl = curl_easy_perform(handler)) != CURLE_OK) {
        // request problem, print err ?
        took = difftime(time(nullptr), time_start);
        return false;
    } else if ((res_data = handlerv_response.str()).length() <= 0) {
        // there is no data, return, print err?
        // "unknown error (1)"
        took = difftime(time(nullptr), time_start);
        return false;
    }
    took = difftime(time(nullptr), time_start);

    cout << res_data << endl;

    // parse the data
    map<string, string> dict = quick_parse(res_data);

    // session start
    {
        bool iserror;
        if ((iserror = dict.find("session_id") != dict.end()))
            session = dict["session_id"];
        return iserror; // true, when no errors
    }

    // session append
    {
        bool iserror;
        if ((iserror = dict.find("error_summary") != dict.end()))
            ;// register error        
        return !iserror; // true, when no errors
    }

    // session finish
    {
        bool iserror;
        if ((iserror = dict.find("error_summary") != dict.end()))
            ;// register error        
        return !iserror; // true, when no errors
    }

    // clean up the handler and return
    curl_easy_cleanup(handler);

    return false;
}

/* uploader */

bool uploader(const string& token, const path_t& file, long& took) {
    vector<const char*> memblocks;
    size_t remaining = file_split(file.lo, file.size, CKSIZE, memblocks);

    // single file upload, up to 150, 
    if (memblocks.size() == 1) return upload(token, file, took);
    
    // session file upload, chunks of 150MB, 1K = 1024

    string session;
    size_t offset = 0; // last offset
    long ttook; // temporal time holder
    char* nodata = nullptr;

    // reset the chunk progress
    progress_data.chunk_total = memblocks.size();
    progress_data.chunk_current = 0;

    // start session
    upload_session(token, session, nodata, 0, ttook, file, session_start);
    took += ttook;

    // session append
    for (int i = 0; i < memblocks.size(); i++) {
        progress_data.chunk_current++;
        upload_session(token, session, memblocks[i], offset, ttook, file, 
                       session_append);
        took += ttook;
        offset += (i == memblocks.size() - 1 && remaining ? remaining : CKSIZE);  
    }

    upload_session(token, session, nodata, offset, ttook, file, session_finish);
    took += ttook;

    return false;
}

void display_help(bool detail = false) {
        const string helpcmd = R"(Usage : throwfile [OPTION] path
Upload your local files to Dropbox
For more help, use  -h option)";

    if (detail) {
        cout << helpcmd << endl;
        // cout << "Required:" << endl;
        cout << "Optional:" << endl;
        cout << "  -d      : dry run, will not upload, just verbose";
        cout << "  -r path : remote path starts with / and ends without /, ";
        cout << "e.g.: /remotepath, default one is /tmp/throwfile" << endl;
        cout << "this is useful to test if this client is valid"  << endl;
    }
}

/* main */

int main(int argc, char* argv[]) {
    /*    
    -d       : dry run (optional)
    -r <arg> : the remote folder to upload the data
    path to process 
    */

    // options parser

    // options
    
    string path;
    string remote_dir = "/tmp/throwfile";
    bool dryrun = false;

    int opt = -1;
    while ((opt = getopt(argc, argv, "hdr:")) != -1) {
        switch(opt) {
        case 'd':
            dryrun = true;
            break;
        case 'r':
            remote_dir = optarg;
            if (remote_dir[0] != '/') {
                display_help(true);
                return EXIT_FAILURE;
            } else if (remote_dir[remote_dir.size() - 1] == '/') {
                display_help(true);
                return EXIT_FAILURE;
            }
            break;
        case 'h':
            display_help(true);
            return EXIT_SUCCESS;
        case '?':
            display_help();
            return EXIT_FAILURE;
        default:
            display_help();
            return EXIT_FAILURE;
        }
    }

    if (optind >= argc) {
        display_help();
        return EXIT_FAILURE;
    }

    // path, as the last option
    path = argv[optind];
    while (path[path.size() - 1] == '/' && path.size() > 1) 
        path = path.substr(0, path.size() - 1); 

    // -- end of the options section --

    // token validation

    string token;
    bool success;
    string res;

    // on error exit
    if (!(success = oauth2_token_get(token))) {
        cout << "error(token)(get)" << endl;
        return EXIT_FAILURE;
    } else if (!(success = oauth_token_validate(token, res))) {
        cout << "error(token)(validate)" << endl;
        return EXIT_FAILURE;
    }

    cerr << "Good to see you " << res << endl;

    // list files
    
    vector<path_t> files;
    ls(path, files, remote_dir);

    for_each(files.begin(), files.end(), [&dryrun, &token](const path_t& file) {
        long took = 0;
        long remain = 0;
        long pieces = 0;

        remain = file_split_sizes(file.size, CKSIZE, pieces);

        cout << "+" << endl;
        cout << "|from local : " << file.lo << endl;
        cout << "|to remote  : " << file.re << endl;
        cout << "|size       : " << size_unit(file.size) << endl;
        cout << "|other      : " << pieces << " x " << size_unit(CKSIZE);
        cout << " + " << size_unit(remain) << endl;

        if (!dryrun) uploader(token, file, took);
        cout << "|took       : " <<  time_unit(took) << endl;

    });
    cout << "+" << endl;

    return EXIT_SUCCESS;
}

// -------------------------------------------------------------------------- //
/* disclaimer, license and stuff */

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

#define _150MB (1000 * 1000 * 150) // K as 1000

/* namespaces */
using namespace std;

/* structs */
struct path_t {
    string lo; // local
    string re; // remote
    size_t size;
};

struct progress_data_t {
    double speed_sum;
    int speed_count;
    bool done;
    CURL* handler;
};

/* others */
enum session_mode { single, session_start, session_append, session_finish };

/* typedefs */
typedef struct path_t path_t;
typedef struct progress_data_t progress_data_t;

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
        // strerror(errno)
        return;
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
        char* block_mem = new char[block_size];
        file.read(block_mem, block_size);
        holder.push_back(block_mem);
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

    // cout << "resdata: \n" << res_data << endl;

    // clean up the handler and return
    curl_easy_cleanup(handler);

    return true; 
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

    if (auth_input.empty()) {
        // error cout << "can't continue with no auth or token code..." << endl;
        return false;
    }

    // perform the request

    // return on error
    if (!oauth2_token_remote_get(token, auth_input)) {
        return false;
    }

    // persist the token for the future, and return the completion
    return oauth2_token_file_set(token);
}

/* upload */

bool upload_session_start(const string& token, const path_t& file) {
    return false;
}

bool upload_session_end(const string& token, const path_t& file) {
    return false;
}

bool upload_session(const string& token, const path_t& file) {
    return false;
}

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
    cerr << "|progress   : " << percent << "%";
    
    if (percent < 100) {
        cerr << " at " << size_unit(speed) << "/sec";
        cerr << "\r";
    } else {
        double avgs = progress_data->speed_sum / progress_data->speed_count;
        cerr << " (" << size_unit(avgs) << "/sec avg)" << endl;
    }

    cerr << std::flush;

    // update struct
    progress_data->done = percent >= 100;
    progress_data->speed_sum += speed;
    progress_data->speed_count++; 
    
    return 0;
}

bool upload(const string& token, const path_t& file, int& took) {
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
    vector<const char*> memblocks;
    size_t remaining = file_split(file.lo, file.size, _150MB, memblocks);

    const char* memblock = memblocks[0]; // check this with valgrind from memleaks

    curl_easy_setopt(handler, CURLOPT_URL, EP_UPLOAD);
    curl_easy_setopt(handler, CURLOPT_WRITEFUNCTION, handlerf_response);
    curl_easy_setopt(handler, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(handler, CURLOPT_POSTFIELDS, memblock);

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
    string res_data;

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

    return true; 
}

/* upload session */

// API

// EP_UPLOAD_SESSION_START -----------------------------------------------------
// params:
// {
//     "close": false
// }
// returns:
// {
//     "session_id": "1234faaf0678bcde"
// }
// errors nothing!

// EP_UPLOAD_SESSION_APPEND ----------------------------------------------------
// params:
// {
//     "cursor": {
//         "session_id": "1234faaf0678bcde",
//         "offset": 0
//     },
//     "close": false
// }
// returns nothing!
// errors error_summary

// EP_UPLOAD_SESSION_FINISH ----------------------------------------------------
// params:
// {
//     "cursor": {
//         "session_id": "1234faaf0678bcde",
//         "offset": 0
//     },
//     "commit": {
//         "path": "/Homework/math/Matrices.txt",
//         "mode": "add",
//         "autorename": true,
//         "mute": false
//     }
// }
// returns:
// {
//     "name": "Prime_Numbers.txt",
//     "id": "id:a4ayc_80_OEAAAAAAAAAXw",
//     "client_modified": "2015-05-12T15:50:38Z",
//     "server_modified": "2015-05-12T15:50:38Z",
//     "rev": "a1c10ce0dd78",
//     "size": 7212,
//     "path_lower": "/homework/math/prime_numbers.txt",
//     "path_display": "/Homework/math/Prime_Numbers.txt",
//     "sharing_info": {
//         "read_only": true,
//         "parent_shared_folder_id": "84528192421",
//         "modified_by": "dbid:AAH4f99T0taONIb-OurWxbNQ6ywGRopQngc"
//     },
//     "property_groups": [
//         {
//             "template_id": "ptid:1a5n2i6d3OYEAAAAAAAAAYa",
//             "fields": [
//                 {
//                     "name": "Security Policy",
//                     "value": "Confidential"
//                 }
//             ]
//         }
//     ],
//     "has_explicit_shared_members": false,
//     "content_hash": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
// }
// errors: error_summary

bool upload_session(const string& token, const char* memblock, int& took, 
                    const int mode) {
    CURL* handler;

    if (!(handler = curl_easy_init())) {
        // print err?
        return false;
    }

    string endpoint;
    string api = "Dropbox-API-Arg: ";
    
    if (mode == session_start) {
        // session start
        api += "";
        endpoint = EP_UPLOAD_SESSION_START;
    } else if (mode == session_append) {
        // session append
        api += "";
        endpoint = EP_UPLOAD_SESSION_APPEND;
    } else if (mode == session_finish) {
        // session finish
        api += "";
        endpoint = EP_UPLOAD_SESSION_FINISH;
    } else {
        // print error
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
    curl_easy_setopt(handler, CURLOPT_POSTFIELDS, memblock);

    // progress

    progress_data.speed_sum = 0;
    progress_data.speed_count = 0;
    progress_data.done = false;
    progress_data.handler = handler;

    curl_easy_setopt(handler, CURLOPT_PROGRESSFUNCTION, progress);
    curl_easy_setopt(handler, CURLOPT_PROGRESSDATA, &progress_data);
    curl_easy_setopt(handler, CURLOPT_NOPROGRESS, 0L);

    return false;
}

/* uploader */

bool uploader(const string& token, const path_t& file, int& took) {
    long remain = 0;
    long pieces = 0;

    remain = file_split_sizes(file.size, _150MB, pieces);

    return false;
}

/* main */

int main(int argc, char* argv[]) {
    // add cmd validation
    string help = "";

    // add read from file regex exception

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

    cout << "Good to see you " << res << endl;

    // list files

    string path = real_path("~/Documents");
    
    vector<path_t> files;
    ls(path, files, "/cross");

    for_each(files.begin(), files.end(), [&token](const path_t& file) {
        long took = 0;
        long remain = 0;
        long pieces = 0;

        remain = file_split_sizes(file.size, _150MB, pieces);

        cout << "+" << endl;
        cout << "|from local : " << file.lo << endl;
        cout << "|to remote  : " << file.re << endl;
        cout << "|size       : " << size_unit(file.size) << endl;
        cout << "|to remote  : " << file.re << endl;
        cout << "|other      : " << pieces << " x 150MB + ";
        cout << size_unit(remain) << endl;


        
        //upload(token, file, took);

        cerr << "|took       : " <<  time_unit(took) << endl;
    });
    cout << "+" << endl;

    return EXIT_SUCCESS;
}
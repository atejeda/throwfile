/* disclaimer, license and stuff */

/* compiler and app define */
/*
 gcc.gnu.org/onlinedocs/gcc-5.2.0/libstdc++/manual/manual/using_dual_abi.html
 main.cpp:(.text+0x28): undefined reference to 
 `std::__cxx11::basic_string<char, std::char_traits<char>, 
  std::allocator<char> >::basic_string()'
*/
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

/* c includes (c++) */
#include <cstring>
#include <cstdlib>

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
    "https://content.dropboxapi.com/2/files/upload"

#define EP_UPLOAD_SESSION_UPLOAD \
    "https://content.dropboxapi.com/2/files/upload"

#define EP_UPLOAD_SESSION_END \
    "https://content.dropboxapi.com/2/files/upload"

#define _150MB (1000 * 1000 * 150)

/* namespaces */
using namespace std;

/* structs */
struct fpath_t {
    string lo; // local
    string re; // remote
    size_t size;
};

/* typedef */

typedef struct fpath_t fpath_t;

/* application variables */

// application credentials
const static string app_key = "y5dfulqv9ayvjty";
const static string app_secret = "b0ucqd3w9a9tqja";

// handles the responses from the handler
static stringstream handlerv_response;

/* function declarations */

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

void ls(const string path, vector<fpath_t>& files, const string parent) {
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
            files.push_back(fpath_t{lo, re, static_cast<size_t>(size)});
        } else if (S_ISDIR(entry_stat.st_mode)) {
            if (name == ".." || name == ".")
                continue;
            ls(lo, files, parent + '/' + root);
        }
    }

    closedir(dir);
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

    const long pieces = size / csize;
    const long remain = size % csize;

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

string file_unit(const size_t size) {
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

bool upload_session_start(const string& token, const fpath_t& file) {
    return false;
}

bool upload_session_end(const string& token, const fpath_t& file) {
    return false;
}

bool upload_session(const string& token, const fpath_t& file) {
    return false;
}

bool upload(const string& token, const fpath_t& file) {
// {  
// "name":"acsStartContainer_javaContainer_2017-06-03_21.53.08.711",
// "path_lower":
//   "/cross/tmp3/acsstartcontainer_javacontainer_2017-06-03_21.53.08.711",
// "path_display":
//    "/cross/tmp3/acsStartContainer_javaContainer_2017-06-03_21.53.08.711",
// "id":"id:DstbCk1UZpAAAAAAAABrmg",
// "client_modified":"2017-06-18T08:07:23Z",
// "server_modified":"2017-06-18T08:07:24Z",
// "rev":"559550280a6a7",
// "size":1552301,
// "content_hash":
//    "2b4bd110227f10643095ee29893f72b9022324d0e87e90f22ccc0bceab6672a9"
// }

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
    api += "{\"path\":\"" + file.re;
    api += + "\", \"mode\":\"overwrite\"}";
    headers = curl_slist_append(headers, api.c_str());

    // file setup
    vector<const char*> memblocks;
    size_t remaining = file_split(file.lo, file.size, _150MB, memblocks);

    const char* post = memblocks[0]; // check this with valgrind from memleaks

    curl_easy_setopt(handler, CURLOPT_URL, EP_UPLOAD);
    curl_easy_setopt(handler, CURLOPT_WRITEFUNCTION, handlerf_response);
    curl_easy_setopt(handler, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(handler, CURLOPT_POSTFIELDS, post);

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

bool uploader(const string& token, const fpath_t& file) {
    return false;
}

/* main */

int main(int argc, char* argv[]) {
    string help = "";

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

    string path = real_path("~/Desktop/tmp3");
    
    vector<fpath_t> files;
    ls(path, files, "/cross");

    for_each(files.begin(), files.end(), [&token](const fpath_t& file) {
        cout << "-> upload " << file.lo << " " << file_unit(file.size) << endl;
        cout << "   to " << file.re << endl;
        upload(token, file);
    });

    return EXIT_SUCCESS;
}
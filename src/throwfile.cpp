#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include <cmath>

#include "customtypes.h"
#include "restclient.h"
#include "tokenmgr.h"
#include "utils.h"
#include "uploadmgr.h"

using namespace std;

const string app_key = "y5dfulqv9ayvjty";
const string app_secret = "b0ucqd3w9a9tqja";

restclient* rest_client = new restclient();

completion_t validate_token(const string token) {
    tokenmgr token_mgr;
    completion_t completion = token_mgr.validate(rest_client, token);
    return completion;
}

string get_token(bool force_request = false) {
    // return a completion instead

    string token;
    tokenmgr token_mgr;
    completion_t completion;

    completion = token_mgr.get_from_file();

    if (!completion.completion || force_request) {
        string url = "https://www.dropbox.com/oauth2/authorize";
        url += "?client_id=" + app_key;
        url += "&response_type=code";

        string auth;

        cout << "Token configuration is empty or doesn't exists:" << endl;
        cout << "To authorize this application,";
        cout << "copy this address below in your browser, ";
        cout << "then copy the provided auth code here" << endl;
        cout << "\e[1m" << url << "\e[0m" << endl;
        cout << "Paste the auth code here : ";
        cin >> auth;

        if (auth.empty()) {
            cout << "Can't continue with no auth or token code...";
        }

        completion = token_mgr.get_from_request(rest_client, auth, app_key, app_secret);

        if (completion.completion) {
            token = completion.body;
            token_mgr.set_to_file(token);
        } else {
            cout << "There's an error : " << completion.body << endl;
            cout << "Can't continue with no auth or token code..." << endl;
        }
    } else {
        token = completion.body;
    }

    return token;
}

// TODO:
// - validate first argument
// - validate dropbox destination (name)
int main(int argc, char* argv[]) {

    string token;
    completion_t completion_token;
    completion_t completion_validation;

    token = get_token();

    while (true) {
        completion_validation = validate_token(token);

        if (!completion_validation.completion) {
            cout << "There's was an error during the validation : ";
            cout << completion_validation.body << endl;
            cout << "Hit ctrl+c to stop this application, otherwise ";
            cout << "let's try to get a new authentication..." << endl;
            cout << "Press enter to continue..." << endl;
            cin.ignore();
            token = get_token(true);
            EXIT_FAILURE;
        } else {
            cout << "Authorized, good to see you ";
            cout << completion_validation.body << endl;
            break;
        }
    }

    const string path = argv[1];
    const string destination = "/cross";

    cout << endl << "Listing the files, this may take a while depending ";
    cout << "how many files (recursively) the directory contains... " << endl;

    vector<throwfile_path_t>* files_path = new vector<throwfile_path_t>();
    utils::ls(path, files_path, destination);

    cout << endl << files_path->size() << " Files to throw (summary):" << endl;
    cout << "(symlinks ignored)" << endl << endl;

    long total_size = 0;

//    std::for_each(files_path->begin(), files_path->end(), [&total_size](const throwfile_path_t& throw_file) {
//        cout << "from local : " << throw_file.local_path << " " << utils::get_size_unit(throw_file.file_size) << endl;
//        cout << "to dropbox : " << throw_file.remote_path << " ";

//        vector<const char*>* file_chunks;
//        long remaining = utils::split_file(throw_file.local_path, throw_file.file_size, _150MB, &file_chunks);

//        if (file_chunks->size() > 1) {
//            cout << file_chunks->size() << " pieces of 150MB";
//            cout << " and one remaining of " << utils::get_size_unit(remaining) << endl;
//        } else {
//            cout << "1 piece of " << utils::get_size_unit(remaining) << endl;
//        }

//        cout << endl;

//        total_size += throw_file.file_size;
//    });

    cout << endl << utils::get_size_unit(total_size) + " total" << endl << endl;

    // upload files

    uploadmgr* upload_mgr = new uploadmgr();

    upload_mgr->upload_file(rest_client, token, files_path->at(0));


    return EXIT_SUCCESS;
}

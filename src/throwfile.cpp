#include <iostream>
#include <vector>

#include "customtypes.h"
#include "restclient.h"
#include "tokenmgr.h"
#include "utils.h"

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

        completion =
            token_mgr.get_from_request(rest_client, auth, app_key, app_secret);

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

int main(int argc, char* argv[]) {

    // validate first argument
    // resolve the path by using
    // http://man7.org/linux/man-pages/man3/realpath.3.html
    // validate dropbox destination (name)

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

    const string path = argv[1]; // "/home/atejeda/Downloads";
    vector<throwfile_path_t>* files_path = new vector<throwfile_path_t>();

    cout << endl << "Listing the files, this may take a while depending ";
    cout << "how many files (recursively) the directory contains... " << endl;

    utils::ls(path, files_path);

    cout << endl << files_path->size() << " Files to throw:" << endl;
    cout << "(symlinks ignored)" << endl << endl;

    long total_size = 0;

    for (int i = 0; i < files_path->size(); i++) {
        long file_size;
        string file_size_unit;

        long reported_file_size = files_path->at(i).file_size;
        total_size += reported_file_size;

        if ((file_size = reported_file_size / 1000000) > 0) {
            file_size_unit = "MB";
        } else if ((file_size = reported_file_size / 1000) > 0) {
            file_size_unit = "KB";
        } else {
            file_size = reported_file_size;
            file_size_unit = "B";
        }

        cout << "(" << file_size << ") " << file_size_unit << endl;
        cout << "from local : " << files_path->at(i).system_path << endl;
        cout << "to dropbox : " << files_path->at(i).dropbox_path << endl;
        cout << endl;
    }

    cout << endl << total_size / 1000000 << " MB total" << endl << endl;

    return EXIT_SUCCESS;
}

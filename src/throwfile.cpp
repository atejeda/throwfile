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

    return EXIT_SUCCESS;
}

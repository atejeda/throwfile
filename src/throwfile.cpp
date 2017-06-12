#include <iostream>
#include <vector>

#include "customtypes.h"
#include "utils.h"
#include "restclient.h"
#include "tokenmgr.h"

using namespace std;

const string app_key = "y5dfulqv9ayvjty";
const string app_secret = "b0ucqd3w9a9tqja";

string get_token() {
    string token;

    tokenmgr token_manager;
    completion_t token_file_res = token_manager.get_from_file();

    if (!token_file_res.completion) {
       const string url = "https://www.dropbox.com/oauth2/authorize?client_id=" + app_key + "&response_type=code";
       string auth;

       cout << "Token configuration is empty or doesn't exists:" << endl;
       cout << " To authorize this application to interact with your dropbox," << endl;
       cout << " copy this address below in your browser, then copy the provided auth code here" << endl;
       cout << " -> " << "\e[1m" << url << "\e[0m" << endl;
       cout << "Paste the auth code here : ";
       cin >> auth;

       if (auth.empty()) {
           cout << "Can't continue with no auth or token code... exiting..";
       }

       completion_t token_req_res = token_manager.get_from_request(auth, app_key, app_secret);

       if (token_req_res.completion) {
           token = token_req_res.body;
           token_manager.set_to_file(token);
       } else {
           cout << "There's an error : " << token_req_res.body << endl;
           cout << "Can't continue with no auth or token code... exiting.." << endl;
       }
   }

    return token;
}

int main(int argc, char* argv[]) {

    string token = get_token();

    if (token.empty())
       EXIT_FAILURE;

   return EXIT_SUCCESS;
}

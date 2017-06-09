#include <iostream>

#include "connector.h"
#include "customtypes.h"
#include "restclient.h"

using namespace std;

int main(int argc, char* argv[]) {
    restclient* token_client = new restclient();
    connector::token_client = token_client;

    auto handler = connector::token_handler;
    string endpoint = "https://github.com";
    vector<string> headers;
    string postdata;

    completion_t res = token_client->request(handler, endpoint, headers, postdata);
    cout << res.completion << " : " << res.body << endl;
    delete token_client;

    return EXIT_SUCCESS;
}

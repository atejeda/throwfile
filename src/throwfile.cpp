#include <iostream>

#include "customtypes.h"
#include "restclient.h"
#include "connector.h"

using namespace std;

int main(int argc, char* argv[]) {
    restclient* token_client = new restclient();
    connector::token_client = token_client;

    auto handler = connector::token_handler;
    string endpoint = "https://www.google.com";
    vector<string> headers;
    string postdata;

    token_client->request(handler, endpoint, headers, postdata);
    delete token_client;

    return EXIT_SUCCESS;
}

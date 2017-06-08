#include "tokenmgr.h"
#include "customtypes.h"
#include "restclient.h"
#include "connector.h"

tokenmgr::tokenmgr() {

    auto handler = connector::token_handler;
}

completion_t tokenmgr::get_from_file() {

}

completion_t tokenmgr::get_from_request(const string auth) {
    restclient* rest_client = new restclient();
    connector::token_client = rest_client;

    auto handler = connector::token_handler;

    string endpoint = "https://www.google.com";
    vector<string> headers;
    string postdata;

    rest_client->request(handler, endpoint, headers, postdata);
}

tokenmgr::~tokenmgr() {
    delete this->rest_client;
}

//auto handler = connector::token_handler;
//string endpoint = "https://www.google.com";
//vector<string> headers;
//string postdata;
//token_client->request(handler, endpoint, headers, postdata);


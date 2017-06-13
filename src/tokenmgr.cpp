#include <fstream>
#include <iostream>

#include "customtypes.h"
#include "restclient.h"
#include "tokenmgr.h"

using namespace std;

tokenmgr::tokenmgr() {}

completion_t tokenmgr::get_from_file() {
    const string path = "/home/atejeda/.throwfile";
    ifstream file(path);

    completion_t res;

    if ((res.completion = (file.is_open() && file.good()))) {
        string token;
        getline(file, token);
        res.completion = !token.empty();
        res.body = token;
        file.close();
    }

    return res;
}

completion_t tokenmgr::set_to_file(const string token) {
    const string path = "/home/atejeda/.throwfile";
    ofstream file(path);

    completion_t res;

    if ((res.completion = (file.is_open() && file.good()))) {
        file << token;
        file.close();
    }

    return res;
}

completion_t tokenmgr::get_from_request(restclient* rest_client,
                                        const string app_auth,
                                        const string app_key,
                                        const string app_secret) {

    string endpoint = "https://api.dropboxapi.com/oauth2/token";

    vector<string> headers;
    headers.push_back("Content-Type: application/x-www-form-urlencoded;");

    string postdata = "code=" + app_auth;
    postdata += string("&grant_type=authorization_code");
    postdata += "&client_id=" + app_key + "&client_secret=" + app_secret;

    completion_map_t res = rest_client->request(endpoint, headers, postdata);

    completion_t completion;

    map<string, string> dict = res.body;
    bool istoken = dict.find("access_token") != dict.end();
    bool iserror = dict.find("error_description") != dict.end();

    if ((completion.completion = res.completion)) {
        if ((completion.completion = istoken)) {
            completion.body = dict["access_token"];
        } else if (iserror) {
            completion.completion = false;
            completion.body = dict["error_description"];
        } else {
            completion.body = "unknown error (1)";
        }
    } else {
        if (iserror) {
            completion.body = dict["error_description"];
        } else {
            completion.body = "unknown error (2)";
        }
    }

    return completion;
}

completion_t tokenmgr::validate(restclient* rest_client, const string token) {
    string endpoint = "https://api.dropboxapi.com/1/account/info";

    vector<string> headers;
    headers.push_back("Authorization: Bearer " + token);

    string postdata;
    completion_map_t res = rest_client->request(endpoint, headers, postdata);

    completion_t completion;

    map<string, string> dict = res.body;
    bool istoken = dict.find("display_name") != dict.end();
    bool iserror1 = dict.find("error_description") != dict.end();
    bool iserror2 = dict.find("error") != dict.end();

    if ((completion.completion = res.completion)) {
        if ((completion.completion = istoken)) {
            completion.body = dict["display_name"];
        } else if (iserror1) {
            completion.completion = false;
            completion.body = dict["error_description"];
        } else if (iserror2) {
            completion.completion = false;
            completion.body = dict["error"];
        } else {
            completion.body = "unknown error (1)";
        }
    } else {
        if (iserror1) {
            completion.body = dict["error_description"];
        } else if (iserror2) {
            completion.body = dict["error"];
        } else {
            completion.body = "unknown error (2)";
        }
    }

    return completion;
}

tokenmgr::~tokenmgr() {}

#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>

#include "customtypes.h"
#include "mongoose.h"
#include "restclient.h"
#include "utils.h"

using namespace std;

restclient::restclient(const connection_manager_t& connection_manager) {
    this->connection_manager = connection_manager;
}

restclient::restclient() : handler_flag(false) {
    mg_mgr_init(&connection_manager, NULL);
    this->connection_manager = connection_manager;
}

completion_map_t restclient::request(const string url_s, const vector<string>& headers_v, const string post_s) {
    restclient::static_client = this;

    this->handler_flag = false;

    string headers_s;

    std::for_each(headers_v.begin(), headers_v.end(),
                  [&headers_s](const string& header) { headers_s += header + "\n"; });

    // headers_s += "\r\n";

    const char* url = url_s.c_str();
    const char* headers = headers_s.size() ? headers_s.c_str() : nullptr;
    const char* post = post_s.size() ? post_s.c_str() : nullptr;

    // cout << "url    : " << url << endl;
    // cout << "headers :" << endl << headers << endl;
    // cout << "post   : " << post << endl;

    mg_connect_http(&this->connection_manager, restclient::static_handler, url, headers, post);

    while (!this->handler_flag)
        mg_mgr_poll(&this->connection_manager, 1000);

    // process the response

    completion_map_t res;

    auto resdata = this->handler_res.response;
    // cout << resdata << endl;
    map<string, string> restdict;

    try {
        restdict = utils::quick_parse(resdata);
        res.completion = restdict.find("error_description") == restdict.end();
        res.body = restdict;
    } catch (...) {
        restdict["error_description"] = "there's an error : \n" + resdata;
        res.completion = false;
        res.body = restdict;
    }

    return res;
}

restclient* restclient::static_client;
void restclient::static_handler(connection_t* nc, int ev, void* ev_data) {
    restclient::static_client->handler(nc, ev, ev_data);
}

void restclient::handler(connection_t* nc, int ev, void* ev_data) {
    http_message_t* res = (http_message_t*)ev_data;

    // sent data https://docs.cesanta.com/mongoose/master/#/http/client_example.md/
    // struct mg_connection has void *user_data which is a placeholder for application-specific data.
    // Mongoose does not
    // use that pointer. Event handler can store any kind of information there.
    // https://docs.cesanta.com/mongoose/master/#/c-api/net.h/struct_mg_connection.md/



    switch (ev) {
    case MG_EV_POLL:
        //        try {
        //            cout << string(nc->send_mbuf.buf).size() << endl;
        //        } catch (...) {
        //            ;
        //        }

        try {
            cout << *(int*)ev_data << endl;
        } catch (...) {
            ;
        }

        break;
    case MG_EV_SEND: // data has been sent
        // cout << "MG_EV_SEND " << ev << endl;
        //        try {
        //            cout << string(nc->send_mbuf.buf).size() << endl;
        //        } catch (...) {
        //            ;
        //        }

        try {
            cout << *(int*)ev_data << endl;
        } catch (...) {
            ;
        }

        break;
    case MG_EV_CONNECT:
        if (*(int*)ev_data != 0) {
            cout << "connection failed, " << strerror(*(int*)ev_data) << endl;
            this->handler_res.code = -1;
            nc->flags |= MG_F_SEND_AND_CLOSE;
            this->handler_flag = true;
        }
        break;
    case MG_EV_HTTP_REPLY:
        this->handler_res.message = static_cast<string>(res->message.p);
        this->handler_res.code = res->resp_code;
        this->handler_res.response = static_cast<string>(res->body.p);
        nc->flags |= MG_F_CLOSE_IMMEDIATELY;
        this->handler_flag = true;
        break;
    case MG_EV_CLOSE:
        if (!this->handler_flag) {
            cout << "closed connection" << endl;
            this->handler_flag = true;
        }
        break;
    default:
        break;
    }
}

restclient::~restclient() {
    this->handler_flag = false;
    mg_mgr_free(&this->connection_manager);
}

#include <iostream>
#include <sstream>
#include <algorithm>

#include "mongoose.h"
#include "customtypes.h"
#include "restclient.h"

using namespace std;

restclient::restclient(const connection_manager_t& connection_manager) : handler_flag(false) {
    this->connection_manager = connection_manager;
}

restclient::restclient() : handler_flag (false) {
    mg_mgr_init(&connection_manager, NULL);
    this->connection_manager = connection_manager;
}

void restclient::request(const mg_event_handler_t& handler, string endpoint, vector<string> headers, string postdata) {
    this->handler_flag = false;

    string extra_headers;
    // TODO: refactor this
    for (int i = 0; i < headers.size(); i++) {
        extra_headers += headers[i];
        if (i == headers.size() - 1)
            extra_headers += "\r\n";
    }

    const char* c_endpoint = endpoint.c_str();
    const char* c_headers =  extra_headers.size() ? extra_headers.c_str() : nullptr;
    const char* c_post = postdata.size() ? postdata.c_str() : nullptr;

    mg_connect_http(&this->connection_manager, handler, c_endpoint, c_headers, c_post);

    while (!this->handler_flag)
        mg_mgr_poll(&this->connection_manager, 1000);

    cout << this->handler_response.response << endl;
}

void restclient::handler(connection_t* nc, int ev, void* ev_data) {
    http_message_t* response = (http_message_t*) ev_data;

    switch (ev) {
        case MG_EV_CONNECT:
            if (*(int*) ev_data != 0) {
                cout << "connection failed, " << strerror(*(int*) ev_data) << endl;
                 this->handler_response.code = -1;
                nc->flags |= MG_F_SEND_AND_CLOSE;
                this->handler_flag = true;
            }
            break;
        case MG_EV_HTTP_REPLY:
             this->handler_response.message = static_cast<string>(response->message.p);
             this->handler_response.code = response->resp_code;
             this->handler_response.response = static_cast<string>(response->body.p);
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

#include "WebSocketServer.h"

#define ASIO_STANDALONE 
#define ASIO_HEADER_ONLY

#define _WEBSOCKETPP_CPP11_STL_

//#include <asio.hpp>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <set>

websocketpp::server<websocketpp::config::asio> server;
std::mutex m_connection_lock;
std::set<websocketpp::connection_hdl,std::owner_less<websocketpp::connection_hdl>> connections;

WebSocketServer::WebSocketServer() {
    
}

WebSocketServer::~WebSocketServer() {
    
}

void WebSocketServer::open() {
    server.set_message_handler([this](websocketpp::connection_hdl hdl, websocketpp::server<websocketpp::config::core>::message_ptr msg) {
        if (msg->get_opcode() == websocketpp::frame::opcode::text) {
            auto message = msg->get_payload();
            messageRead(message);
        }
    });

    server.set_open_handler([this](websocketpp::connection_hdl hdl) {
        std::lock_guard guard(m_connection_lock);
        connections.insert(hdl);
        bool hasClients = !connections.empty();
        onClientConnectedStateChanged(hasClients);
    });

    server.set_close_handler([this](websocketpp::connection_hdl hdl) {
        std::lock_guard guard(m_connection_lock);
        connections.erase(hdl);
        bool hasClients = !connections.empty();
        onClientConnectedStateChanged(hasClients);
    });

    server.init_asio();
    server.listen(9002);
    server.start_accept();
    server.run();
}

void WebSocketServer::close() {
    server.stop();
    server.stop_listening();
}

void WebSocketServer::writeMessage(std::string message) {
    std::lock_guard guard(m_connection_lock);

    for (auto& it : connections) {
        server.send(it, message, websocketpp::frame::opcode::text);
    }
}

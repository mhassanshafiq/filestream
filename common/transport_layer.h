#pragma once
#include <iostream>
#include <winsock2.h>
#include <functional>
#include "packet.h"
#include <thread>
#include <mutex>
#include <ws2tcpip.h>
#include <unordered_map>
#pragma comment(lib,"Ws2_32.lib")


#define BUFFER_SIZE (64 * 1024)

class transport_layer
{
public:
    transport_layer();
    ~transport_layer();
    void start_server(int port);
    bool connect_to_server(std::string host, int port);
    void disconnect_from_server();
    bool send_data_to_client(SOCKET client, std::string remote_host, std::shared_ptr<packet> packet_obj);
    bool send_data_to_server(std::shared_ptr<packet> packet_obj) { return send_data(server, packet_obj); }
    void add_on_connect_handler(std::function<void(SOCKET client_sock, std::string remote_host)> handler);
    void add_on_disconnect_handler(std::function<void(SOCKET client_sock, std::string remote_host)> handler);
    void add_on_receive_handler(std::function<void(SOCKET client_sock, std::shared_ptr<packet> packet_obj)> handler);
    void listener(SOCKET client, std::string remote_host);
    void stop_server();
    void close_socket(SOCKET sock);
    bool is_connected_to_server() const { return connected_to_server; }
private:
    bool send_data(SOCKET sock, std::shared_ptr<packet> packet_obj);
    WSADATA WSAData;
    SOCKET server;
    
    bool running = false;
    bool connected_to_server = false;
    bool is_server = false;
    std::unordered_map<SOCKET, std::string> connected_clients_map;
    std::mutex connected_clients_lock;

    std::function<void(SOCKET client, std::shared_ptr<packet> pck)> m_on_receive_handler;
    std::function<void(SOCKET client, std::string remote_host)> m_on_connect_handler;
    std::function<void(SOCKET client, std::string remote_host)> m_on_disconnect_handler;

    void remove_client(SOCKET client);
	
};
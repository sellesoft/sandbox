//qol typedef's
typedef zed_net_socket_t zsocket;
typedef zed_net_address_t zaddress;

zsocket csocket;
zaddress caddress;

b32 is_server = 0;
b32 is_client = 0;

//opens the socket on specified port
//this is internal, but maybe have more uses later
b32 net_open_socket(u64 port){
    if(zed_net_udp_socket_open(&csocket, port, 1)){
        LogE("Net", "Socket failed to open on port ", port,  " with error ", zed_net_get_error());
        return 0;
    }
    LogS("Net", "Successfully opened UDP socket on port ", port);
    return 1;
}

//initializes the network to act as a client
//returns false if connecting failed
b32 net_init_client(str8 host, u64 port){
    zed_net_init();
    is_client = 1;
    net_open_socket(port);
    if(zed_net_get_address(&caddress, (char*)host.str, port)){
        LogE("net","Connecting to host '", host, "' failed with error:\n", zed_net_get_error()); 
        zed_net_socket_close(&csocket);
        return 0;
    }
    LogS("net", "Successfully connected to host '", host, "' on port '", port, "'");
    return 1;
}

//cleans up the networking
void net_deinit(){
    //if(is_client) zed_net_socket_close(&client_socket);
    //if(is_server) zed_net_socket_close(&server_socket);
    zed_net_socket_close(&csocket);
    is_client = is_server = 0;
    zed_net_shutdown();
} 

//sends data to the client's connected server
b32 net_client_send(NetInfo info){
    Assert(is_client, "Can't send info as client unless network is initialized as a client");
    if(zed_net_udp_socket_send(&csocket, caddress, &info, sizeof(NetInfo))){
        LogE("net", "Failed to send data with error:\n", zed_net_get_error());
        return 0;
    }
    return 1;
}

NetInfo net_client_recieve(){
    zaddress sender;
    NetInfo info;
    s32 bytes_read = zed_net_udp_socket_receive(&csocket, &sender, &info, sizeof(NetInfo));
	Logf("net","Received %d bytes from %s:%d", bytes_read, zed_net_host_to_str(sender.host), sender.port);
    if(bytes_read == -1){
        LogE("net", "Client failed to read bytes with error:\n", zed_net_get_error());
        return {0};
    }
    if(!bytes_read) info.magic[0] = 0; 
    return info;
}

u32 host_phase = 0;

b32 net_host_game(){
	switch(host_phase){
        case 0:{//set index and broadcast host message
            player_idx = 0;
            NetInfo info;
            info.uid = 0;
            info.message = Message_HostGame;
            net_client_send(info);
        }break;
        case 1:{//wait for response from another client
            NetInfo info = net_client_recieve();
            if(info.message == Message_JoinGame){
                NetInfo info; //acknowledge join game
                info.uid = 0;
                info.message = Message_AcknowledgeMessage;
                net_client_send(info);
                return false;
            }else{ //rebroadcast message TODO(sushi) check if this is necessary
                info = NetInfo();
                info.uid = 0;
                info.message = Message_HostGame;
                net_client_send(info);
            }
        }break;
    }
    return true;
}

u32 join_phase = 0;

b32 net_join_game(){
    switch(join_phase){
        case 0:{ //searching for HostGame message, respond if found
            player_idx = 1;
            NetInfo info = net_client_recieve();
            if(info.message == Message_HostGame){
                info.uid = 1;
                info.message = Message_JoinGame;
                net_client_send(info);
                join_phase = 1;
            }
        }break;            
        case 1:{ // we have found HostGame message and responded, now we wait for the server to acknowledge us joining 
            NetInfo info = net_client_recieve();
            if(info.message == Message_AcknowledgeMessage) return join_phase = 0, false;
        }break;
    }
    return true;
}

#if 0
//recieves a message from some client and then rebroadcasts it to all other clients
NetInfo net_server_recieve(){
    zaddress sender;
    NetInfo ninfo;
    s32 bytes_read = zed_net_udp_socket_receive(&server_socket, &sender, &ninfo, sizeof(NetInfo));
    if(bytes_read == -1) LogE("net", "Server receieve failed with error:\n",zed_net_get_error());
    return ninfo;
}


b32 net_server_send(NetInfo info, zaddress address){
    if(zed_net_udp_socket_send(&server_socket, address, &info, sizeof(NetInfo))){
        LogE("net", "Server failed to send data with error:\n",zed_net_get_error());
        return 0;
    }
    return 1;
}

b32 net_init_server(u64 port){
    is_server = 1;
    return 0;
}
#endif
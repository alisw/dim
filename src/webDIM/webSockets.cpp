#include "server_ws.hpp"

extern "C"
{
	void ast_conn_h(void *connp, int srvr_conn_id, void **connptr);
	void ast_read_h(void *connp, char *msg);
	int ws_open_server(char *name, int port, int nThread, int srvr_conn_id);
	int ws_send_message(void *connptr, char *message, int size);
}

using namespace std;

typedef SimpleWeb::SocketServer<SimpleWeb::WS> WsServer;

WsServer server;

int ws_send_message(void *connptr, char *msg, int size)
{
//	auto message = make_shared<WsServer::Message>(msg);
//	auto message_str = message->string();

//	shared_ptr<WsServer::Connection> connection(static_cast<WsServer::Connection*>(connp));
//	shared_ptr<WsServer::Connection> connection = *static_cast <shared_ptr<WsServer::Connection>*>(connp);
	shared_ptr<WsServer::Connection> connection = *static_cast <shared_ptr<WsServer::Connection>*>(connptr);
	cout << "Sending message " << (size_t)connection.get() << endl;
	auto send_stream = make_shared<WsServer::SendStream>();
//	*send_stream << message_str;
	*send_stream << msg;
	//server.send is an asynchronous function
	server.send(connection, send_stream, [](const boost::system::error_code& ec){
		if (ec) {
			cout << "Server: Error sending message. " <<
				//See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
				"Error: " << ec << ", error message: " << ec.message() << endl;
		}
	});
	return 1;
}

void ws_connection_received(shared_ptr<WsServer::Connection> connection, int srvr_conn_id)
{
//	shared_ptr<WsServer::Connection> connection1(new WsServer::Connection(connection));
//	connection1 = connection;
//	ast_conn_h(static_cast<void*>(connection.get()), srvr_conn_id);
	shared_ptr<WsServer::Connection>* connptr = new shared_ptr<WsServer::Connection>(connection);
//	void* my_void = conn_ptr;
	ast_conn_h(static_cast<void*>(connection.get()), srvr_conn_id, (void **)&connptr);
//	ast_conn_h(static_cast<void*>(connection.get()), srvr_conn_id);
}

void ws_connection_closed(shared_ptr<WsServer::Connection> connection)
{
//	shared_ptr<WsServer::Connection>* conn_ptr = new shared_ptr<WsServer::Connection>(connection);
//	void* my_void = conn_ptr;
//	ast_conn_h(my_void, 0);
	void *connptr;
	ast_conn_h(static_cast<void*>(connection.get()), 0, &connptr);
//	shared_ptr<WsServer::Connection> connection = *static_cast <shared_ptr<WsServer::Connection>*>(connptr);
	delete static_cast <shared_ptr<WsServer::Connection>*>(connptr);
}

void ws_message_received(shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::Message> message)
{
	//WsServer::Message::string() is a convenience function for:
	//stringstream data_ss;
	//data_ss << message->rdbuf();
	//auto message_str = data_ss.str();
	auto message_str = message->string();

	cout << "Server: Message received: \"" << message_str << "\" from " << (size_t)connection.get() << endl;
//	shared_ptr<WsServer::Connection>* conn_ptr = new shared_ptr<WsServer::Connection>(connection);
//	void* my_void = conn_ptr;
//	ast_read_h(my_void, (char *)message_str.c_str());
	ast_read_h(static_cast<void*>(connection.get()), (char *)message_str.c_str());
/*
	cout << "Server: Sending message \"" << message_str << "\" to " << (size_t)connection.get() << endl;

	auto send_stream = make_shared<WsServer::SendStream>();
	*send_stream << message_str;
	//server.send is an asynchronous function
	server.send(connection, send_stream, [](const boost::system::error_code& ec){
		if (ec) {
			cout << "Server: Error sending message. " <<
				//See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
				"Error: " << ec << ", error message: " << ec.message() << endl;
		}
	});
*/
}

//int ws_send_message()

int ws_open_server(char *name, int port, int nThread, int srvr_conn_id) {
	char sname[128];
	server.config.port=port;
    
    //Example 1: echo WebSocket endpoint
    //  Added debug messages for example use of the callbacks
    //  Test with the following JavaScript:
    //    var ws=new WebSocket("ws://localhost:8080/echo");
    //    ws.onmessage=function(evt){console.log(evt.data);};
    //    ws.send("test");
	strcpy(sname, "^/");
	strcat(sname, name);
	strcat(sname, "/?$");
	auto& dimClient = server.endpoint[sname];
    
	dimClient.on_message = [](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::Message> message) {
		ws_message_received(connection, message);
    };
    
	dimClient.on_open = [&srvr_conn_id](shared_ptr<WsServer::Connection> connection) {
        cout << "Server: Opened connection " << (size_t)connection.get() << endl;
		ws_connection_received(connection, srvr_conn_id);
	};
    
    //See RFC 6455 7.4.1. for status codes
	dimClient.on_close = [](shared_ptr<WsServer::Connection> connection, int status, const string& /*reason*/) {
        cout << "Server: Closed connection " << (size_t)connection.get() << " with status code " << status << endl;
		ws_connection_closed(connection);
	};
    
    //See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
	dimClient.on_error = [](shared_ptr<WsServer::Connection> connection, const boost::system::error_code& ec) {
        cout << "Server: Error in connection " << (size_t)connection.get() << ". " << 
                "Error: " << ec << ", error message: " << ec.message() << endl;
    };
    
    thread server_thread([](){
        //Start WS-server
        server.start();
    });
    
    //Wait for server to start so that the client can connect
//    this_thread::sleep_for(chrono::seconds(1));
    
    server_thread.join();
    
    return 0;
}


//
// Created by lzb on 19-8-10.
//
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <libdevcore/Log.h>
#include <json/value.h>
#include <json_spirit/JsonSpiritHeaders.h>
#include <libweb3jsonrpc/JsonHelper.h>
#include <libbrcdchain/Transaction.h>
#include <libdevcore/CommonIO.h>
#include <microhttpd.h>
#include "ToolTransaction.h"
#include "WalletServer.h"


using namespace dev;
using namespace std;

namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;
namespace js = json_spirit;
namespace dbt = dev::brc::transationTool;


int main(int argc, char *argv[]) {

    try {
        bpo::options_description description("command line ");
        description.add_options()
                ("help,h", "show help message.")
                ("port,p", bpo::value<int>(), "http listen port.")
                ("send,s", bpo::value<std::string>(),"get the http ip and port, use this option will auto to send rawTransation to http host...");

        bpo::variables_map args_map;
        bpo::parsed_options parsed = bpo::parse_command_line(argc, argv, description);
        bpo::store(parsed, args_map);

        int port =8088;
        std::string _url ="";

        if (args_map.count("help")) {
            std::cout << description << std::endl;
            return 0;
        }
        if (args_map.count("port")) {
           port = args_map["port"].as<int>();
        }

        if (args_map.count("send")) {
            _url = args_map["send"].as<std::string>();
        }

        HttpServer server(port);
        wallet::WalletServer w_server(server, _url);

        if (w_server.StartListening()) {
            cout << "Server started successfully ...Listening:" << port<< endl;
            getchar();
            //w_server.StopListening();
        } else {
            cout << "Error starting Server" << endl;
        }
        while (true)
        {
            usleep(500);
        }

    }

    catch (jsonrpc::JsonRpcException &e){
        std::cout<< e.what() << std::endl;
        exit(1);
    }
    catch (...) {

    }
}
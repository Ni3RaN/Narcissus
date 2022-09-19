#include "config.h"

int main(int argc, char *argv[]) {
    std::string user = "test";
    std::string passwd = "test";
    std::string databasename = "test";

    Config config;

    config.parse_arg(argc,argv);

    WebServer webServer;

    webServer.init(config.PORT, user, passwd, databasename, config.LOGWrite,
                        config.OPT_LINGER, config.TRIGMode,  config.sql_num,  config.thread_num,
                        config.close_log);


    webServer.log_write();

    printf("Server port: %d\n", config.PORT);
    webServer.sql_pool();
    webServer.thread_pool();
    webServer.trig_mode();
    webServer.eventListen();
    webServer.eventLoop();

    return 0;
}
#include "config.h"

int main(int argc, char *argv[]) {

    Config config;

    WebServer webServer;

    webServer.init(config.port, config.mysql_port, config.mysql_url, config.mysql_username, config.mysql_password,
                   config.mysql_database,
                   config.log_write,
                   config.opt_linger, config.trig_mode, config.sql_num, config.thread_num,
                   config.close_log, config.timer_mode);


    webServer.log_write();

    printf("Server port: %d\n", config.port);

    webServer.sql_pool();
    webServer.thread_pool();
    webServer.trig_mode();
    webServer.eventListen();
    webServer.eventLoop();

    return 0;
}
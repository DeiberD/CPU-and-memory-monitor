#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "structs.h"

// Read memory information from /proc/meminfo and return it
char* get_values_from_meminfo(short PORT, char* agent_ip) {

    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) {
        perror("fopen /proc/meminfo");
        return NULL;
    }

    char line[256];
    unsigned int mem_total_kb = 0, mem_available_kb = 0, mem_free_kb = 0, SwapTotal_kb = 0, SwapFree_kb = 0;

    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "MemTotal: %u kB", &mem_total_kb) == 1) continue;
        if (sscanf(line, "MemFree: %u kB", &mem_free_kb) == 1) continue;
        if (sscanf(line, "MemAvailable: %u kB", &mem_available_kb) == 1) continue;
        if (sscanf(line, "SwapTotal: %u kB", &SwapTotal_kb) == 1) continue;
        if (sscanf(line, "SwapFree: %u kB", &SwapFree_kb) == 1) continue;
    }
    fclose(f);

    unsigned int mem_used_mb = (mem_total_kb - mem_available_kb) / 1024;

    // Fortmatted output
    static char values[320];
    int len = snprintf(values, sizeof(values),
        "MEM;"
        "%s;"
        "%u;"
        "%u;"
        "%u;"
        "%u;",
        agent_ip,
        mem_used_mb, 
        mem_free_kb / 1024.0,
        SwapTotal_kb / 1024.0,
        SwapFree_kb / 1024.0);

    return values;
}

// Connect to server and return socket descriptor
int connect_to_server(char* collector_ip, short PORT) {
    int agentfd, r;
    struct sockaddr_in server_addr;

    // Socket creation
    agentfd = socket(AF_INET, SOCK_STREAM, 0);
    if(agentfd < 0){
        perror("\n--> Error en la creacion del socket");
        exit(-1);
    }

    // Server address setup
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, collector_ip, &server_addr.sin_addr);

    // Connect to server
    r = connect(agentfd, (struct sockaddr *)&server_addr, (socklen_t)sizeof(struct sockaddr));
    if(r < 0){
        perror("\n--> Error en connect(): ");
        exit(-1);
    }

    return agentfd;
}

int main(int argc, char *argv[]){
    // Arguments
    const char *server_ip = argv[1];
    short port = atoi(argv[2]);
    const char *agent_ip = argv[3];

    int agentfd = connect_to_server(server_ip, port);
    char* values;

    // Send a message to the server in a loop 
    while(1){
        values = get_values_from_meminfo(port, (char*) agent_ip);
        send(agentfd, values, strlen(values), 0);
        sleep(2); 
    }

    close(agentfd);
    return 0;
}
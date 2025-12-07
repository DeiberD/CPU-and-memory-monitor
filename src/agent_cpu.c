#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

// Read CPU information from /proc/stat and return it
// Calcula el porcentaje de uso de CPU entre dos lecturas

struct cpu_times {
    unsigned long user, nice, system, idle, iowait, irq, softirq;
};

char* get_values_from_cpuinfo(short PORT, char* agent_ip) {

    // Valores previos para calcular diferenciales
    static struct cpu_times prev = {0};
    struct cpu_times curr = {0};

    FILE *f = fopen("/proc/stat", "r");
    if (!f) {
        perror("fopen /proc/stat");
        return NULL;
    }

    // Leer la línea que empieza por "cpu"
    fscanf(f, "cpu %lu %lu %lu %lu %lu %lu %lu",
           &curr.user, &curr.nice, &curr.system,
           &curr.idle, &curr.iowait,
           &curr.irq, &curr.softirq);
    fclose(f);

    // Calcular totales y tiempos ociosos
    unsigned long prev_idle = prev.idle + prev.iowait;
    unsigned long idle = curr.idle + curr.iowait;

    unsigned long prev_non_idle = prev.user + prev.nice + prev.system +
                                  prev.irq + prev.softirq;
    unsigned long non_idle = curr.user + curr.nice + curr.system +
                             curr.irq + curr.softirq;

    unsigned long prev_total = prev_idle + prev_non_idle;
    unsigned long total = idle + non_idle;

    unsigned long total_delta = total - prev_total;
    unsigned long idle_delta = idle - prev_idle;

    // Actualizar lectura previa
    struct cpu_times old = prev;
    prev = curr;

    // Evitar división por cero
    if (total_delta == 0) total_delta = 1;

    // Calcular porcentajes
    double cpu_usage = 100.0 * (total_delta - idle_delta) / total_delta;
    double user_pct = 100.0 * (curr.user - old.user) / total_delta;
    double system_pct = 100.0 * (curr.system - old.system) / total_delta;
    double idle_pct = 100.0 * idle_delta / total_delta;

    // Formatted output
    static char values[320];
    snprintf(values, sizeof(values),
        "CPU;"
        "%s;"
        "%.2f;"
        "%.2f;"
        "%.2f;"
        "%.2f;",
        agent_ip,
        cpu_usage,
        user_pct,
        system_pct,
        idle_pct
    );

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
    r = connect(agentfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
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

    int agentfd = connect_to_server((char*)server_ip, port);
    char* values;

    // Loop sending data
    while(1){
        values = get_values_from_cpuinfo(port, (char*) agent_ip);
        send(agentfd, values, strlen(values), 0);
        sleep(2);
    }

    close(agentfd);
    return 0;
}

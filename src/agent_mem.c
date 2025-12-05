#include <stdio.h>
#include <string.h>
#include "structs.h"


// Read memory information from /proc/meminfo and return it
struct mem_values get_value_from_meminfo() {

    static struct mem_values values;

    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) {
        perror("fopen /proc/meminfo");
        return values;
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

    // Store values in struct
    values.mem_total_kb = mem_total_kb;
    values.mem_available_kb = mem_available_kb;
    values.mem_free_kb = mem_free_kb;
    values.SwapTotal_kb = SwapTotal_kb;
    values.SwapFree_kb = SwapFree_kb; 

    return values;
}

int main(void){
    // Read memory information from /proc/meminfo
    
    struct mem_values meminfo = get_value_from_meminfo();
    printf("%u", meminfo.mem_available_kb);
    
    return 0;
}
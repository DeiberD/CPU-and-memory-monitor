# Monitor Distribuido de CPU y Memoria

Sistema de monitoreo distribuido que recopila y visualiza métricas de CPU y memoria de múltiples máquinas remotas en tiempo real, utilizando sockets TCP y lectura directa de `/proc` en Linux.

## Autores

- **[@daguilastro](https://github.com/daguilastro)** - Daniel Aguilar Castro
- **[@feliariasg](https://github.com/feliariasg)** - Andres Felipe Arias Gonzalez
- **[@DeiberD](https://github.com/DeiberD)** - Deiber Gongora Hurtado


## Descripción

Este proyecto implementa un sistema cliente-servidor que permite monitorear el estado de hasta 4 máquinas remotas desde una consola central. El sistema lee información directamente del sistema de archivos `/proc` de Linux y la transmite mediante sockets TCP.

### Componentes del Sistema

1. **Agente de Memoria (`agent_mem`)**: Monitorea el uso de memoria RAM y swap.
2. **Agente de CPU (`agent_cpu`)**: Monitorea el uso del procesador.
3. **Recolector Central (`collector`)**: Recibe y almacena datos de múltiples agentes.
4. **Visualizador**: Muestra en tiempo real una tabla con las métricas de todas las máquinas. Este es un componente del **Recolector Central**.

## Características

- Lectura directa de `/proc/meminfo` y `/proc/stat`.
- Comunicación mediante sockets TCP.
- Soporte para múltiples agentes concurrentes.
- Actualización en tiempo real.
- Visualización en tabla formatada.
- Manejo de errores y desconexiones.
- Código modular y bien documentado.

## Requisitos

- Sistema operativo: **Linux**
- Compilador: **GCC** o **G++**
- Bibliotecas estándar de C/C++
- Permisos de lectura en `/proc`

## Compilación

```bash
# Compilar el agente de memoria
gcc -o agent_mem agent_mem.c -Wall -Wextra

# Compilar el agente de CPU
gcc -o agent_cpu agent_cpu.c -Wall -Wextra

# Compilar el recolector y visualizador
gcc -o collector collector.c -lpthread -Wall -Wextra
```

O usar el Makefile incluido:

```bash
make all
```

## Uso

### 1. Iniciar el Recolector Central

En la máquina central (puede ser una instancia en la nube):

```bash
./collector <puerto>
```

Ejemplo:
```bash
./collector 8080
```

### 2. Iniciar los Agentes de Memoria

En cada máquina a monitorear:

```bash
./agent_mem <ip_recolector> <puerto> <ip_logica_agente>
```

Ejemplo:
```bash
./agent_mem 192.168.1.100 8080 10.0.0.1
```

### 3. Iniciar los Agentes de CPU

En cada máquina a monitorear:

```bash
./agent_cpu <ip_recolector> <puerto> <ip_logica_agente>
```

Ejemplo:
```bash
./agent_cpu 192.168.1.100 8080 10.0.0.1
```

### Simulación Local

Para probar en una sola máquina con diferentes "IPs lógicas":

```bash
# Terminal 1: Recolector
./collector 8080

# Terminal 2: Agente memoria máquina 1
./agent_mem 127.0.0.1 8080 10.0.0.1

# Terminal 3: Agente CPU máquina 1
./agent_cpu 127.0.0.1 8080 10.0.0.1

# Terminal 4: Agente memoria máquina 2
./agent_mem 127.0.0.1 8080 10.0.0.2

# Terminal 5: Agente CPU máquina 2
./agent_cpu 127.0.0.1 8080 10.0.0.2
```

## Formato de Salida

El visualizador muestra una tabla actualizada cada 2 segundos:

```
┌──────────────┬─────────┬────────────┬───────────┬────────────┬──────────────┬──────────────┐
│      IP      │  CPU%   │ CPU_user%  │ CPU_sys%  │ CPU_idle%  │ Mem_used_MB  │ Mem_free_MB  │
├──────────────┼─────────┼────────────┼───────────┼────────────┼──────────────┼──────────────┤
│  10.0.0.1    │  37.5   │    15.0    │    5.0    │   57.5     │    2048.0    │    1024.0    │
│  10.0.0.2    │  82.1   │    60.0    │   12.0    │   27.9     │    4096.0    │     512.0    │
│  10.0.0.3    │   --    │     --     │    --     │    --      │      --      │      --      │
│  10.0.0.4    │  12.3   │     8.5    │    2.1    │   87.7     │    1536.0    │    2048.0    │
└──────────────┴─────────┴────────────┴───────────┴────────────┴──────────────┴──────────────┘
```

## Protocolo de Comunicación

### Formato de mensajes

**Agente de Memoria:**
```
MEM;<ip_logica>;<mem_used_MB>;<mem_free_MB>;<swap_total_MB>;<swap_free_MB>\n
```

**Agente de CPU:**
```
CPU;<ip_logica>;<cpu_usage>;<user_pct>;<system_pct>;<idle_pct>\n
```

### Ejemplo:
```
MEM;10.0.0.1;2048.5;1024.3;4096.0;4096.0
CPU;10.0.0.1;37.5;15.0;5.0;57.5
```

## Estructura del Proyecto

```
.
├── src 
|    ├── agent_mem.c          # Agente de monitoreo de memoria
|    ├── agent_cpu.c          # Agente de monitoreo de CPU
|    └── collector.c          # Recolector central y visualizador
├── utils
|   ...
├── Makefile             # Archivo de compilación
└── README.md            # Este archivo
```

## Arquitectura

```
┌─────────────────┐         ┌─────────────────┐
│   Máquina 1     │         │   Máquina 2     │
│  ┌───────────┐  │         │  ┌───────────┐  │
│  │agent_mem  │──┼────┐    │  │agent_mem  │──┼────┐
│  └───────────┘  │    │    │  └───────────┘  │    │
│  ┌───────────┐  │    │    │  ┌───────────┐  │    │
│  │agent_cpu  │──┼────┤    │  │agent_cpu  │──┼────┤
│  └───────────┘  │    │    │  └───────────┘  │    │
└─────────────────┘    │    └─────────────────┘    │
                       │                            │
                       │    Sockets TCP             │
                       ▼                            ▼
                ┌──────────────────────────────────────┐
                │      Máquina Central (Nube)          │
                │  ┌────────────────────────────────┐  │
                │  │         Collector              │  │
                │  │  - Recibe conexiones           │  │
                │  │  - Almacena métricas           │  │
                │  │  - Visualiza en tiempo real    │  │
                │  └────────────────────────────────┘  │
                └──────────────────────────────────────┘
```

## Detalles Técnicos

### Lectura de /proc

- **`/proc/meminfo`**: Contiene información sobre memoria RAM y swap
- **`/proc/stat`**: Contiene estadísticas de CPU (primera línea con "cpu")

### Cálculo de Métricas

**Memoria Usada:**
```c
mem_used_MB = (MemTotal - MemAvailable) / 1024
```

**Uso de CPU:**
```c
CPU_total = delta(user + nice + system + idle + iowait + irq + softirq)
CPU_idle = delta(idle)
CPU_usage = 100 * (CPU_total - CPU_idle) / CPU_total
```

### Manejo de Concurrencia

El recolector utiliza hilos (pthreads) o procesos para:
- Aceptar múltiples conexiones simultáneas
- Actualizar el visualizador sin bloquear la recepción de datos

## Manejo de Errores

El sistema maneja los siguientes casos:

- Archivos `/proc` no accesibles
-  Errores de conexión de red
-  Desconexión inesperada de agentes
-  Datos corruptos o mal formateados
-  Límite de conexiones alcanzado

## Pruebas

### Prueba de Carga
```bash
# Lanzar múltiples agentes simultáneamente
for i in {1..4}; do
    ./agent_mem 127.0.0.1 8080 10.0.0.$i &
    ./agent_cpu 127.0.0.1 8080 10.0.0.$i &
done
```

### Prueba de Desconexión
```bash
# Matar un agente y verificar que el sistema continúa
killall agent_mem
```

## Licencia

Este proyecto fue desarrollado como práctica académica para el curso de Sistemas Operativos.

## 🔗 Referencias

- [proc(5) - Linux manual page](https://man7.org/linux/man-pages/man5/proc.5.html)
- [socket(7) - Linux manual page](https://man7.org/linux/man-pages/man7/socket.7.html)
- [pthread(7) - Linux manual page](https://man7.org/linux/man-pages/man7/pthreads.7.html)

---

**Nota**: Este proyecto es solo para fines educativos y no debe usarse en entornos de producción sin las debidas medidas de seguridad y optimizaciones.
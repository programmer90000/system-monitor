#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

volatile sig_atomic_t stop = 0;

void handle_signal(int sig) {
    stop = 1;
}

float read_temperature_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return -1.0;
    }
    
    int temp;
    if (fscanf(file, "%d", &temp) != 1) {
        fclose(file);
        return -1.0;
    }
    
    fclose(file);
    return temp / 1000.0;
}

float get_cpu_temperature() {
    const char *thermal_files[] = {
        "/sys/class/thermal/thermal_zone0/temp",
        "/sys/class/thermal/thermal_zone1/temp",
        "/sys/class/hwmon/hwmon0/temp1_input",
        "/sys/class/hwmon/hwmon1/temp1_input",
        "/sys/class/hwmon/hwmon2/temp1_input",
        NULL
    };
    
    for (int i = 0; thermal_files[i] != NULL; i++) {
        float temp = read_temperature_file(thermal_files[i]);
        if (temp >= 0) {
            return temp;
        }
    }
    
    return -1.0;
}

void print_timestamp() {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    printf("[%02d:%02d:%02d] ", t->tm_hour, t->tm_min, t->tm_sec);
}

int main() {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    printf("CPU Temperature Monitor - Press Ctrl+C to exit\n");
    printf("Time\t\tTemperature (°C)\n");
    printf("----------------------------\n");
    
    while (!stop) {
        float temp = get_cpu_temperature();
        
        print_timestamp();
        if (temp >= 0) {
            printf("CPU Temp: %.1f°C\n", temp);
        } else {
            printf("Error: Could not read temperature\n");
        }
        
        sleep(1); // Update every second
    }
    
    printf("\nMonitoring stopped.\n");
    return 0;
}
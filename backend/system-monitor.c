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

float get_gpu_temperature() {
    const char *gpu_files[] = {
        "/sys/class/drm/card0/device/hwmon/hwmon0/temp1_input",
        "/sys/class/drm/card0/device/hwmon/hwmon1/temp1_input",
        "/sys/class/drm/card1/device/hwmon/hwmon0/temp1_input",
        "/sys/class/drm/card1/device/hwmon/hwmon1/temp1_input",
        "/sys/class/drm/card0/device/hwmon/hwmon2/temp1_input",
        "/sys/class/drm/card0/device/hwmon/hwmon3/temp1_input",
        "/sys/class/drm/card1/device/hwmon/hwmon2/temp1_input",
        "/sys/class/drm/card1/device/hwmon/hwmon3/temp1_input",
        "/sys/class/hwmon/hwmon3/temp1_input",
        "/sys/class/hwmon/hwmon4/temp1_input",
        NULL
    };
    
    for (int i = 0; gpu_files[i] != NULL; i++) {
        float temp = read_temperature_file(gpu_files[i]);
        if (temp >= 0) {
            return temp;
        }
    }
    
    return -1.0;
}

float get_vrm_temperature() {
    const char *vrm_files[] = {
        "/sys/class/hwmon/hwmon0/temp2_input",
        "/sys/class/hwmon/hwmon1/temp2_input",
        "/sys/class/hwmon/hwmon2/temp2_input",
        "/sys/class/hwmon/hwmon3/temp2_input",
        "/sys/class/hwmon/hwmon0/temp3_input",
        "/sys/class/hwmon/hwmon1/temp3_input",
        "/sys/class/hwmon/hwmon2/temp3_input",
        "/sys/class/hwmon/hwmon3/temp3_input",
        "/sys/class/hwmon/hwmon0/in0_input",
        "/sys/class/hwmon/hwmon1/in0_input",
        "/sys/devices/platform/coretemp.0/hwmon/hwmon*/temp2_input",
        "/sys/class/thermal/thermal_zone2/temp",
        "/sys/class/thermal/thermal_zone3/temp",
        NULL
    };

    for (int i = 0; vrm_files[i] != NULL; i++) {
        float temp = read_temperature_file(vrm_files[i]);
        if (temp >= 0) {
            return temp;
        }
    }

    return -1.0;
}

float get_chipset_temperature() {
    const char *chipset_files[] = {
        "/sys/class/hwmon/hwmon0/temp4_input",
        "/sys/class/hwmon/hwmon1/temp4_input",
        "/sys/class/hwmon/hwmon2/temp4_input",
        "/sys/class/hwmon/hwmon3/temp4_input",
        "/sys/class/hwmon/hwmon0/temp5_input",
        "/sys/class/hwmon/hwmon1/temp5_input",
        "/sys/class/hwmon/hwmon2/temp5_input",
        "/sys/class/hwmon/hwmon3/temp5_input",
        "/sys/class/thermal/thermal_zone4/temp",
        "/sys/class/thermal/thermal_zone5/temp",
        "/sys/class/thermal/thermal_zone6/temp",
        "/sys/class/thermal/thermal_zone7/temp",
        NULL
    };

    for (int i = 0; chipset_files[i] != NULL; i++) {
        float temp = read_temperature_file(chipset_files[i]);
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
    
    printf("CPU, GPU, VRM, and Chipset Temperature Monitor - Press Ctrl+C to exit\n");
    printf("Time          CPU Temp (°C)   GPU Temp (°C)   VRM Temp (°C)   Chipset Temp (°C)\n");
    printf("--------------------------------------------------------------------------------\n");
    
    while (!stop) {
        float cpu_temp = get_cpu_temperature();
        float gpu_temp = get_gpu_temperature();
        float vrm_temp = get_vrm_temperature();
        float chipset_temp = get_chipset_temperature();
        
        print_timestamp();
        
        // Print CPU temperature
        if (cpu_temp >= 0) {
            printf("  %5.1f°C      ", cpu_temp);
        } else {
            printf("     N/A       ");
        }
        
        // Print GPU temperature
        if (gpu_temp >= 0) {
            printf("%8.1f°C      ", gpu_temp);
        } else {
            printf("     N/A      ");
        }
        
        // Print VRM temperature
        if (vrm_temp >= 0) {
            printf("%8.1f°C      ", vrm_temp);
        } else {
            printf("     N/A      ");
        }
        
        // Print Chipset temperature
        if (chipset_temp >= 0) {
            printf("%8.1f°C", chipset_temp);
        } else {
            printf("     N/A");
        }
        
        printf("\n");
        
        sleep(1); // Update every second
    }
    
    printf("\nMonitoring stopped.\n");
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <dirent.h>
#include <string.h>

volatile sig_atomic_t stop = 0;

void handle_signal(int sig) {
    stop = 1;
}

typedef struct {
    char name[64];
    char path[256];
} StorageDevice;

StorageDevice *storage_devices = NULL;
int storage_device_count = 0;

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

float get_motherboard_temperature() {
    const char *motherboard_files[] = {
        "/sys/class/hwmon/hwmon0/temp3_input",
        "/sys/class/hwmon/hwmon1/temp3_input",
        "/sys/class/hwmon/hwmon2/temp3_input",
        "/sys/class/hwmon/hwmon3/temp3_input",
        "/sys/class/hwmon/hwmon0/temp6_input",
        "/sys/class/hwmon/hwmon1/temp6_input",
        "/sys/class/hwmon/hwmon2/temp6_input",
        "/sys/class/hwmon/hwmon3/temp6_input",
        "/sys/class/hwmon/hwmon0/temp7_input",
        "/sys/class/hwmon/hwmon1/temp7_input",
        "/sys/class/hwmon/hwmon2/temp7_input",
        "/sys/class/hwmon/hwmon3/temp7_input",
        "/sys/class/thermal/thermal_zone8/temp",
        "/sys/class/thermal/thermal_zone9/temp",
        "/sys/class/thermal/thermal_zone10/temp",
        NULL
    };

    for (int i = 0; motherboard_files[i] != NULL; i++) {
        float temp = read_temperature_file(motherboard_files[i]);
        if (temp >= 0) {
            return temp;
        }
    }

    return -1.0;
}

void find_storage_devices() {
    DIR *dir = opendir("/sys/class/hwmon");
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "hwmon", 5) != 0) continue;

        char name_path[256];
        snprintf(name_path, sizeof(name_path), "/sys/class/hwmon/%s/name", entry->d_name);

        FILE *f = fopen(name_path, "r");
        if (!f) continue;

        char devname[64];
        if (fscanf(f, "%63s", devname) == 1) {
            if (strstr(devname, "nvme") || strstr(devname, "drivetemp")) {
                // Try to get the parent device name (e.g., nvme0)
                char device_path[256];
                snprintf(device_path, sizeof(device_path), "/sys/class/hwmon/%s/device", entry->d_name);

                char real_device[256];
                ssize_t len = readlink(device_path, real_device, sizeof(real_device)-1);
                if (len != -1) {
                    real_device[len] = '\0';
                    // Extract just the last part (like nvme0)
                    char *base = strrchr(real_device, '/');
                    if (base) base++; else base = real_device;

                    // Find first temp*_input
                    for (int i = 1; i < 10; i++) {
                        char temp_path[256];
                        snprintf(temp_path, sizeof(temp_path), "/sys/class/hwmon/%s/temp%d_input", entry->d_name, i);
                        FILE *tf = fopen(temp_path, "r");
                        if (tf) {
                            fclose(tf);
                            
                            // Allocate or reallocate storage devices array
                            StorageDevice *temp = realloc(storage_devices, (storage_device_count + 1) * sizeof(StorageDevice));
                            if (temp == NULL) {
                                fclose(f);
                                closedir(dir);
                                return;
                            }
                            storage_devices = temp;
                            
                            strncpy(storage_devices[storage_device_count].name, base, sizeof(storage_devices[0].name)-1);
                            strncpy(storage_devices[storage_device_count].path, temp_path, sizeof(storage_devices[0].path)-1);
                            storage_device_count++;
                            break;
                        }
                    }
                }
            }
        }
        fclose(f);
    }
    closedir(dir);
}

float get_storage_temperature(const char *path) {
    return read_temperature_file(path);
}

void print_timestamp() {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    printf("[%02d:%02d:%02d] ", t->tm_hour, t->tm_min, t->tm_sec);
}

int main() {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    find_storage_devices();

    printf("CPU, GPU, VRM, Chipset, Motherboard, and Storage Temperature Monitor - Press Ctrl+C to exit\n");
    printf("Time          CPU Temp (°C)   GPU Temp (°C)   VRM Temp (°C)   Chipset Temp (°C)   Motherboard Temp (°C)");
    for (int i = 0; i < storage_device_count; i++) {
        printf("   %s Temp (°C)", storage_devices[i].name);
    }
    printf("\n");
    printf("------------------------------------------------------------------------------------------------------");
    for (int i = 0; i < storage_device_count; i++) {
        printf("----------------");
    }
    printf("\n");

    while (!stop) {
        float cpu_temp = get_cpu_temperature();
        float gpu_temp = get_gpu_temperature();
        float vrm_temp = get_vrm_temperature();
        float chipset_temp = get_chipset_temperature();
        float motherboard_temp = get_motherboard_temperature();
        
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
            printf("%8.1f°C      ", chipset_temp);
        } else {
            printf("     N/A      ");
        }
        
        // Print Motherboard temperature
        if (motherboard_temp >= 0) {
            printf("%12.1f°C", motherboard_temp);
        } else {
            printf("     N/A");
        }

        // Print individual Storage Devices temperature
        for (int i = 0; i < storage_device_count; i++) {
            float temp = get_storage_temperature(storage_devices[i].path);
            if (temp >= 0) printf("   %19.1f°C", temp);
            else printf("     N/A");
        }
        
        printf("\n");
        sleep(1); // Update every second
    }
    
    // Free allocated memory
    free(storage_devices);
    
    printf("\nMonitoring stopped.\n");
    return 0;
}
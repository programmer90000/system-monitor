#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <dirent.h>
#include <string.h>
#include <glob.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/sysinfo.h>
#include <sys/types.h>

volatile sig_atomic_t stop = 0;

void handle_signal(int sig) {
    stop = 1;
}

typedef struct {
    char name[64];
    char path[256];
} StorageDevice;

typedef struct {
    float load_1min;
    float load_5min;
    float load_15min;
} LoadAverage;

typedef struct {
    pid_t pid;
    char name[256];
    char state;
    unsigned long utime;
    unsigned long stime;
    unsigned long starttime;
    long rss;
    long vsz;
    unsigned long read_bytes;
    unsigned long write_bytes;
} ProcessInfo;

typedef struct ProcessNode {
    pid_t pid;
    pid_t ppid;
    char name[256];
    struct ProcessNode* children;
    struct ProcessNode* next;
} ProcessNode;

typedef struct ProcSample {
    pid_t pid;
    unsigned long utime, stime;
} ProcSample;

// CPU Core Monitoring Structures
#define MAX_CORES 32
typedef struct {
    unsigned long user;
    unsigned long nice;
    unsigned long system;
    unsigned long idle;
    unsigned long iowait;
    unsigned long irq;
    unsigned long softirq;
    unsigned long steal;
} CPUStats;

typedef struct {
    char cpu_name[16];
    CPUStats stats;
    CPUStats prev_stats;
    double usage;
} CoreData;

typedef struct {
    int total_cores;
    CoreData cores[MAX_CORES];
    double overall_usage;
} CPUData;

StorageDevice *storage_devices = NULL;
int storage_device_count = 0;
CPUData cpu_data;

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

LoadAverage get_load_average() {
    LoadAverage load = {-1.0, -1.0, -1.0};
    FILE *file = fopen("/proc/loadavg", "r");
    if (file == NULL) {
        return load;
    }
    
    if (fscanf(file, "%f %f %f", &load.load_1min, &load.load_5min, &load.load_15min) != 3) {
        fclose(file);
        load.load_1min = load.load_5min = load.load_15min = -1.0;
        return load;
    }
    
    fclose(file);
    return load;
}

// Function to get core count
int get_core_count() {
    FILE *file = fopen("/proc/stat", "r");
    if (!file) {
        return -1;
    }

    char line[256];
    int count = -1; // Start at -1 to exclude the total 'cpu' line

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "cpu", 3) == 0) {
            count++;
        }
    }

    fclose(file);
    return count;
}

// Function to read CPU stats for all cores
void read_cpu_stats(CPUData *cpu_data) {
    FILE *file = fopen("/proc/stat", "r");
    if (!file) {
        return;
    }

    char line[256];
    int core_count = 0;

    // Store previous stats for calculation (skip if it's the first run)
    static int first_run = 1;
    if (!first_run) {
        for (int i = 0; i <= cpu_data->total_cores; i++) {
            cpu_data->cores[i].prev_stats = cpu_data->cores[i].stats;
        }
    }

    while (fgets(line, sizeof(line), file) && core_count <= cpu_data->total_cores) {
        if (strncmp(line, "cpu", 3) == 0) {
            CPUStats stats;
            char cpu_label[16];
            
            int matched = sscanf(line, 
                "%15s %lu %lu %lu %lu %lu %lu %lu %lu",
                cpu_label,
                &stats.user, &stats.nice, &stats.system, &stats.idle,
                &stats.iowait, &stats.irq, &stats.softirq,
                &stats.steal);

            if (matched >= 4) {
                strncpy(cpu_data->cores[core_count].cpu_name, cpu_label, sizeof(cpu_data->cores[core_count].cpu_name));
                cpu_data->cores[core_count].stats = stats;
                core_count++;
            }
        }
    }

    fclose(file);
    
    if (first_run) {
        first_run = 0;
        // For first run, set previous stats equal to current stats
        for (int i = 0; i <= cpu_data->total_cores; i++) {
            cpu_data->cores[i].prev_stats = cpu_data->cores[i].stats;
        }
    }
}

// Function to calculate CPU usage for all cores
void calculate_cpu_usage(CPUData *cpu_data) {
    for (int i = 0; i <= cpu_data->total_cores; i++) {
        CPUStats *current = &cpu_data->cores[i].stats;
        CPUStats *previous = &cpu_data->cores[i].prev_stats;

        unsigned long prev_total = previous->user + previous->nice + previous->system +
                                 previous->idle + previous->iowait + previous->irq +
                                 previous->softirq + previous->steal;

        unsigned long current_total = current->user + current->nice + current->system +
                                    current->idle + current->iowait + current->irq +
                                    current->softirq + current->steal;

        unsigned long prev_idle = previous->idle + previous->iowait;
        unsigned long current_idle = current->idle + current->iowait;

        unsigned long total_diff = current_total - prev_total;
        unsigned long idle_diff = current_idle - prev_idle;

        if (total_diff > 0) {
            cpu_data->cores[i].usage = 100.0 * (total_diff - idle_diff) / total_diff;
            
            // Ensure usage is within bounds
            if (cpu_data->cores[i].usage < 0.0) cpu_data->cores[i].usage = 0.0;
            if (cpu_data->cores[i].usage > 100.0) cpu_data->cores[i].usage = 100.0;
        } else {
            cpu_data->cores[i].usage = 0.0;
        }
    }

    // Overall usage is the first core (cpu, not cpu0, cpu1, etc.)
    cpu_data->overall_usage = cpu_data->cores[0].usage;
}

// Function to display CPU usage table
void display_cpu_table(CPUData *cpu_data) {
    // Clear screen and move cursor to top
    printf("\033[2J\033[H");
    
    printf("╔══════════════════════════════════════════════════╗\n");
    printf("║                 CPU USAGE MONITOR                ║\n");
    printf("╠═══════════════╦══════════════════════════════════╣\n");
    printf("║     CPU       ║           USAGE %%                ║\n");
    printf("╠═══════════════╬══════════════════════════════════╣\n");
    
    // Display overall CPU usage
    printf("║ %-13s ║", "Overall");
    
    int overall_bars = (int)(cpu_data->overall_usage / 3.33);
    if (overall_bars > 30) overall_bars = 30;
    if (overall_bars < 0) overall_bars = 0;
    
    for (int j = 0; j < 30; j++) {
        printf(j < overall_bars ? "█" : " ");
    }
    printf(" %5.1f%% ║\n", cpu_data->overall_usage);
    
    printf("╠═══════════════╬══════════════════════════════════╣\n");
    
    // Display individual core usage (start from 1, since 0 is the overall)
    for (int i = 1; i <= cpu_data->total_cores; i++) {
        printf("║ %-13s ║", cpu_data->cores[i].cpu_name);
        
        int bars = (int)(cpu_data->cores[i].usage / 3.33);
        if (bars > 30) bars = 30;
        if (bars < 0) bars = 0;
        
        for (int j = 0; j < 30; j++) {
            printf(j < bars ? "█" : " ");
        }
        printf(" %5.1f%% ║\n", cpu_data->cores[i].usage);
    }
    
    printf("╚═══════════════╩══════════════════════════════════╝\n");
    printf("\nPress Ctrl+C to exit...\n");
}

float get_cpu_usage() {
    static unsigned long long last_total = 0, last_idle = 0;
    FILE *file = fopen("/proc/stat", "r");
    if (file == NULL) {
        return -1.0;
    }
    
    char line[256];
    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return -1.0;
    }
    
    fclose(file);
    
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    if (sscanf(line, "cpu  %llu %llu %llu %llu %llu %llu %llu %llu", 
               &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) != 8) {
        return -1.0;
    }
    
    unsigned long long total = user + nice + system + idle + iowait + irq + softirq + steal;
    unsigned long long total_diff = total - last_total;
    unsigned long long idle_diff = idle - last_idle;
    
    float usage = 0.0;
    if (total_diff > 0 && last_total > 0) {
        usage = 100.0 * (1.0 - (float)idle_diff / total_diff);
    }
    
    last_total = total;
    last_idle = idle;
    
    return usage;
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

float get_psu_temperature() {    
    DIR *dir = opendir("/sys/class/hwmon");
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strncmp(entry->d_name, "hwmon", 5) != 0) continue;

            char name_path[256];
            snprintf(name_path, sizeof(name_path), "/sys/class/hwmon/%s/name", entry->d_name);

            FILE *f = fopen(name_path, "r");
            if (!f) continue;

            char devname[64];
            if (fscanf(f, "%63s", devname) == 1) {
                // Check if this might be a PSU monitoring device
                if (strstr(devname, "psu") || strstr(devname, "PSU") || 
                    strstr(devname, "corsair") || strstr(devname, "Corsair") ||
                    strstr(devname, "seasonic") || strstr(devname, "Seasonic") ||
                    strstr(devname, "evga") || strstr(devname, "EVGA") ||
                    strstr(devname, "bequiet") || strstr(devname, "BeQuiet") ||
                    strstr(devname, "rm850x") || strstr(devname, "RM850x") ||
                    strstr(devname, "hx1000") || strstr(devname, "HX1000")) {
                    
                    // Try to find temperature sensors
                    for (int i = 1; i <= 5; i++) {
                        char temp_path[256];
                        snprintf(temp_path, sizeof(temp_path), 
                                "/sys/class/hwmon/%s/temp%d_input", entry->d_name, i);
                        
                        float temp = read_temperature_file(temp_path);
                        if (temp >= 0) {
                            fclose(f);
                            closedir(dir);
                            return temp;
                        }
                    }
                }
            }
            fclose(f);
        }
        closedir(dir);
    }
    
    const char *psu_files[] = {
        "/sys/class/hwmon/hwmon*/temp1_input",
        "/sys/class/hwmon/hwmon*/temp2_input",
        "/sys/class/hwmon/hwmon*/temp3_input",
        "/sys/class/hwmon/hwmon*/temp4_input",
        "/sys/class/hwmon/hwmon*/temp5_input",
        "/sys/class/hwmon/hwmon*/power*/temp1_input",
        "/sys/class/hwmon/hwmon*/power*/temp2_input",
        NULL
    };
    
    for (int i = 0; psu_files[i] != NULL; i++) {
        glob_t glob_result;
        if (glob(psu_files[i], GLOB_NOSORT, NULL, &glob_result) == 0) {
            for (size_t j = 0; j < glob_result.gl_pathc; j++) {
                float temp = read_temperature_file(glob_result.gl_pathv[j]);
                if (temp >= 0) {
                    globfree(&glob_result);
                    return temp;
                }
            }
            globfree(&glob_result);
        }
    }
    
    dir = opendir("/sys/class/power_supply");
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;
            
            char temp_path[256];
            snprintf(temp_path, sizeof(temp_path), 
                    "/sys/class/power_supply/%s/temp", entry->d_name);
            
            float temp = read_temperature_file(temp_path);
            if (temp >= 0) {
                closedir(dir);
                return temp;
            }
            
            // Check for temperature in hwmon subdirectory
            char hwmon_path[256];
            snprintf(hwmon_path, sizeof(hwmon_path), 
                    "/sys/class/power_supply/%s/hwmon*/temp1_input", entry->d_name);
            
            glob_t glob_result;
            if (glob(hwmon_path, GLOB_NOSORT, NULL, &glob_result) == 0) {
                for (size_t j = 0; j < glob_result.gl_pathc; j++) {
                    temp = read_temperature_file(glob_result.gl_pathv[j]);
                    if (temp >= 0) {
                        globfree(&glob_result);
                        closedir(dir);
                        return temp;
                    }
                }
                globfree(&glob_result);
            }
        }
        closedir(dir);
    }
    
    return -1.0;
}

float get_case_temperature() {
    const char *case_files[] = {
        "/sys/class/hwmon/hwmon*/temp3_input",
        "/sys/class/hwmon/hwmon*/temp4_input",
        "/sys/class/hwmon/hwmon*/temp5_input",
        "/sys/class/hwmon/hwmon*/temp6_input",
        "/sys/class/hwmon/hwmon*/temp7_input",
        "/sys/class/hwmon/hwmon*/temp8_input",
        "/sys/class/hwmon/hwmon*/temp9_input",
        "/sys/class/hwmon/hwmon*/temp10_input",
        "/sys/class/thermal/thermal_zone*/temp",
        "/sys/devices/platform/nct6775.*/hwmon/hwmon*/temp*_input",
        "/sys/devices/platform/it87.*/hwmon/hwmon*/temp*_input",
        "/sys/devices/platform/f71882fg.*/hwmon/hwmon*/temp*_input",
        NULL
    };

    DIR *dir = opendir("/sys/class/hwmon");
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strncmp(entry->d_name, "hwmon", 5) != 0) continue;

            for (int i = 1; i <= 10; i++) {
                char label_path[256];
                snprintf(label_path, sizeof(label_path), 
                        "/sys/class/hwmon/%s/temp%d_label", entry->d_name, i);
                
                FILE *f = fopen(label_path, "r");
                if (f) {
                    char label[64];
                    if (fscanf(f, "%63s", label) == 1) {
                        if (strstr(label, "case") || strstr(label, "Case") || 
                            strstr(label, "CASE") || strstr(label, "ambient") ||
                            strstr(label, "Ambient") || strstr(label, "AMBIENT")) {
                            
                            char temp_path[256];
                            snprintf(temp_path, sizeof(temp_path), 
                                    "/sys/class/hwmon/%s/temp%d_input", entry->d_name, i);
                            
                            float temp = read_temperature_file(temp_path);
                            fclose(f);
                            if (temp >= 0) {
                                closedir(dir);
                                return temp;
                            }
                        }
                    }
                    fclose(f);
                }
            }
        }
        closedir(dir);
    }

    for (int i = 0; case_files[i] != NULL; i++) {
        glob_t glob_result;
        if (glob(case_files[i], GLOB_NOSORT, NULL, &glob_result) == 0) {
            for (size_t j = 0; j < glob_result.gl_pathc; j++) {
                float temp = read_temperature_file(glob_result.gl_pathv[j]);
                if (temp >= 0) {
                    globfree(&glob_result);
                    return temp;
                }
            }
            globfree(&glob_result);
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
            if (strstr(devname, "nvme") || strstr(devname, "drivetemp") || 
                strstr(devname, "sata") || strstr(devname, "SATA") ||
                strstr(devname, "ssd") || strstr(devname, "SSD")) {
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
}

void *monitor_system(void *arg) {
    find_storage_devices();

    for (int i = 0; i < storage_device_count; i++) {
    }
    for (int i = 0; i < storage_device_count; i++) {
    }

    // Initialize CPU core monitoring
    cpu_data.total_cores = get_core_count();
    if (cpu_data.total_cores <= 0) {
        printf("Error: Could not determine number of CPU cores\n");
        return NULL;
    }

    // Initial read to populate stats
    read_cpu_stats(&cpu_data);
    sleep(2); // Wait for initial data

    while (!stop) {
        float cpu_usage = get_cpu_usage();
        LoadAverage load = get_load_average();
        float cpu_temp = get_cpu_temperature();
        float gpu_temp = get_gpu_temperature();
        float vrm_temp = get_vrm_temperature();
        float chipset_temp = get_chipset_temperature();
        float motherboard_temp = get_motherboard_temperature();
        float psu_temp = get_psu_temperature();
        float case_temp = get_case_temperature();
        
        // Read and display CPU core usage
        read_cpu_stats(&cpu_data);
        calculate_cpu_usage(&cpu_data);
        // display_cpu_table(&cpu_data);
        
        print_timestamp();
        
        // Print CPU usage
        if (cpu_usage >= 0) {
        } else {
        }
        
        // Print Load averages
        if (load.load_1min >= 0 && load.load_5min >= 0 && load.load_15min >= 0) {
        } else {
        }
        
        // Print CPU temperature
        if (cpu_temp >= 0) {
        } else {
        }
        
        // Print GPU temperature
        if (gpu_temp >= 0) {
        } else {
        }
        
        // Print VRM temperature
        if (vrm_temp >= 0) {
        } else {
        }
        
        // Print Chipset temperature
        if (chipset_temp >= 0) {
        } else {
        }
        
        // Print Motherboard temperature
        if (motherboard_temp >= 0) {
        } else {
        }
        
        // Print PSU temperature
        if (psu_temp >= 0) {
        } else {
        }

        // Print Case temperature
        if (case_temp >= 0) {
        } else {
        }

        // Print individual Storage Devices temperature
        for (int i = 0; i < storage_device_count; i++) {
            float temp = get_storage_temperature(storage_devices[i].path);
            if (temp >= 0) {}
            else {}
        }
        

        
        sleep(1); // Update every second
    }
    
    // Free allocated memory
    free(storage_devices);
}

static long get_total_jiffies() {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return -1;
    char cpu[16];
    long user, nice, system, idle, iowait, irq, softirq, steal;
    fscanf(f, "%s %ld %ld %ld %ld %ld %ld %ld %ld",
           cpu, &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
    fclose(f);
    return user + nice + system + idle + iowait + irq + softirq + steal;
}

static void read_process_stat(pid_t pid, unsigned long *utime, unsigned long *stime, long *rss) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE *f = fopen(path, "r");
    if (!f) { *utime = *stime = 0; *rss = 0; return; }

    int p;
    char comm[256], state;
    unsigned long tmp;
    fscanf(f, "%d %s %c", &p, comm, &state);
    for (int i = 0; i < 11; i++) fscanf(f, "%lu", &tmp);
    fscanf(f, "%lu %lu", utime, stime);
    for (int i = 0; i < 7; i++) fscanf(f, "%lu", &tmp);
    fscanf(f, "%ld", rss);
    fclose(f);
}

static void read_process_io(pid_t pid, unsigned long *read_bytes, unsigned long *write_bytes) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/io", pid);
    FILE *f = fopen(path, "r");
    *read_bytes = *write_bytes = 0;
    if (!f) return;
    char key[64];
    unsigned long val;
    while (fscanf(f, "%63s %lu", key, &val) == 2) {
        if (strcmp(key, "read_bytes:") == 0) *read_bytes = val;
        if (strcmp(key, "write_bytes:") == 0) *write_bytes = val;
    }
    fclose(f);
}

void list_processes() {
    long page_size = sysconf(_SC_PAGESIZE);
    int ncpu = sysconf(_SC_NPROCESSORS_ONLN);

    ProcessNode* process_table[32768] = {NULL};
    ProcessNode* root = NULL;
    DIR *dir;
    struct dirent *entry;

    dir = opendir("/proc");
    if (!dir) {
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (!isdigit(entry->d_name[0])) continue;
        pid_t pid = atoi(entry->d_name);

        char path[256];
        snprintf(path, sizeof(path), "/proc/%s/comm", entry->d_name);
        FILE *f = fopen(path, "r");
        if (!f) continue;

        char name[256];
        if (!fgets(name, sizeof(name), f)) {
            fclose(f);
            continue;
        }
        fclose(f);
        name[strcspn(name, "\n")] = '\0';

        pid_t ppid = 0;
        snprintf(path, sizeof(path), "/proc/%s/status", entry->d_name);
        f = fopen(path, "r");
        if (f) {
            char line[128];
            while (fgets(line, sizeof(line), f)) {
                if (strncmp(line, "PPid:", 5) == 0) {
                    ppid = atoi(line + 5);
                    break;
                }
            }
            fclose(f);
        }

        ProcessNode* node = (ProcessNode*)malloc(sizeof(ProcessNode));
        node->pid = pid;
        node->ppid = ppid;
        strncpy(node->name, name, sizeof(node->name) - 1);
        node->name[sizeof(node->name) - 1] = '\0';
        node->children = NULL;
        node->next = NULL;

        if (pid < 32768) {
            process_table[pid] = node;
        }
    }
    closedir(dir);

    for (int i = 0; i < 32768; i++) {
        if (!process_table[i]) continue;
        ProcessNode* child = process_table[i];
        pid_t parent_pid = child->ppid;
        if (parent_pid > 0 && parent_pid < 32768 && process_table[parent_pid]) {
            child->next = process_table[parent_pid]->children;
            process_table[parent_pid]->children = child;
        } else if (child->pid == 1) {
            root = child;
        }
    }

    // === snapshot 1 ===
    long total1 = get_total_jiffies();
    ProcSample samples[32768] = {0};

    for (int i = 0; i < 32768; i++) {
        if (!process_table[i]) continue;
        unsigned long ut, st;
        long rss;
        read_process_stat(i, &ut, &st, &rss);
        samples[i].pid = i;
        samples[i].utime = ut;
        samples[i].stime = st;
    }

    usleep(200000); // 200ms

    // === snapshot 2 ===
    long total2 = get_total_jiffies();
    long total_diff = total2 - total1;

    // recursive print
    void print_tree(ProcessNode* node, int depth) {
        if (!node) return;
        unsigned long ut, st, rb, wb;
        long rss;
        read_process_stat(node->pid, &ut, &st, &rss);
        read_process_io(node->pid, &rb, &wb);

        double cpu_percent = 0.0;
        if (samples[node->pid].pid == node->pid) {
            unsigned long du = ut - samples[node->pid].utime;
            unsigned long ds = st - samples[node->pid].stime;
            cpu_percent = ((double)(du + ds) / (double)total_diff) * 100.0 * ncpu;
        }

        for (int i = 0; i < depth; i++) printf("  ");
        printf("%-20s PID=%-5d CPU=%5.2f%% MEM=%-8ldKB IO[R=%-8lu W=%-8lu\n",
               node->name,
               node->pid,
               cpu_percent,
               (rss * page_size) / 1024,
               rb,
               wb);

        print_tree(node->children, depth + 1);
        print_tree(node->next, depth);
    }

    // cleanup
    void free_tree(ProcessNode* node) {
        if (!node) return;
        free_tree(node->children);
        free_tree(node->next);
        free(node);
    }

    if (root) {
        free_tree(root);
    }
}


void *process_thread(void *arg) {
    while (!stop) {
        list_processes();
        sleep(5); // Update every 5 seconds
    }
    return NULL;
}

int main() {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    pthread_t monitor_tid, process_tid;

    pthread_create(&monitor_tid, NULL, monitor_system, NULL);
    pthread_create(&process_tid, NULL, process_thread, NULL);

    pthread_join(monitor_tid, NULL);
    pthread_join(process_tid, NULL);

    
    return 0;
}
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
#include <sys/stat.h>
#include <sys/utsname.h>
#include <gnu/libc-version.h>
#include <sys/statvfs.h>
#include <pwd.h>

volatile sig_atomic_t stop = 0;

void handle_signal(int sig) {
    stop = 1;
}

// Storage device structure to hold name and temperature sensor path
typedef struct {
    char name[64];
    char path[256];
} StorageDevice;

// Load average structure for 1, 5, and 15 minute system loads
typedef struct {
    float load_1min;
    float load_5min;
    float load_15min;
} LoadAverage;

// Process information structure containing detailed process metrics
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

// Process tree node structure for building hierarchical process relationships
typedef struct ProcessNode {
    pid_t pid;
    pid_t ppid;
    char name[256];
    struct ProcessNode* children;
    struct ProcessNode* next;
} ProcessNode;

// Process sample structure for CPU usage calculation between time intervals
typedef struct ProcSample {
    pid_t pid;
    unsigned long utime, stime;
} ProcSample;

// CPU Core Monitoring Structures
#define MAX_CORES 32

// CPU statistics structure for tracking various CPU time states
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

// Core data structure containing CPU name, current and previous stats, and usage percentage
typedef struct {
    char cpu_name[16];
    CPUStats stats;
    CPUStats prev_stats;
    double usage;
} CoreData;

// CPU data structure containing overall usage and individual core information
typedef struct {
    int total_cores;
    CoreData cores[MAX_CORES];
    double overall_usage;
} CPUData;

typedef struct {
    unsigned long user;
    unsigned long nice;
    unsigned long system;
    unsigned long idle;
    unsigned long iowait;
    unsigned long irq;
    unsigned long softirq;
    unsigned long steal;
} CPUUtilization;

// History tracking
#define HISTORY_SIZE 10

// Circular buffer structure for storing historical metric values
typedef struct {
    float values[HISTORY_SIZE];
    int index;
    int count;
} HistoryBuffer;

// Comprehensive system history structure tracking all monitored metrics
typedef struct {
    HistoryBuffer cpu_usage;
    HistoryBuffer load_1min;
    HistoryBuffer load_5min;
    HistoryBuffer load_15min;
    HistoryBuffer cpu_temp;
    HistoryBuffer gpu_temp;
    HistoryBuffer vrm_temp;
    HistoryBuffer chipset_temp;
    HistoryBuffer motherboard_temp;
    HistoryBuffer psu_temp;
    HistoryBuffer case_temp;
    HistoryBuffer storage_temps[16];
    HistoryBuffer core_usage[MAX_CORES];
    HistoryBuffer total_processes;
    int storage_count;
} SystemHistory;

struct cpu_stats {
    unsigned long long utime;
    unsigned long long stime;
    unsigned long long total_time;
};

// Global variables
StorageDevice *storage_devices = NULL;
int storage_device_count = 0;
CPUData cpu_data;
SystemHistory system_history;
void init_history_buffer(HistoryBuffer *buffer) {
    memset(buffer->values, 0, sizeof(buffer->values));
    buffer->index = 0;
    buffer->count = 0;
}
void add_to_history(HistoryBuffer *buffer, float value) {
    buffer->values[buffer->index] = value;
    buffer->index = (buffer->index + 1) % HISTORY_SIZE;
    if (buffer->count < HISTORY_SIZE) {
        buffer->count++;
    }
}

float get_history_average(HistoryBuffer *buffer) {
    if (buffer->count == 0) return 0.0;
    
    float sum = 0.0;
    for (int i = 0; i < buffer->count; i++) {
        sum += buffer->values[i];
    }
    return sum / buffer->count;
}

/**
 * Initializes all history buffers in the system history structure
 * Sets up tracking for all monitored system metrics
 */
void init_system_history() {
    init_history_buffer(&system_history.cpu_usage);
    init_history_buffer(&system_history.load_1min);
    init_history_buffer(&system_history.load_5min);
    init_history_buffer(&system_history.load_15min);
    init_history_buffer(&system_history.cpu_temp);
    init_history_buffer(&system_history.gpu_temp);
    init_history_buffer(&system_history.vrm_temp);
    init_history_buffer(&system_history.chipset_temp);
    init_history_buffer(&system_history.motherboard_temp);
    init_history_buffer(&system_history.psu_temp);
    init_history_buffer(&system_history.case_temp);
    init_history_buffer(&system_history.total_processes);
    
    for (int i = 0; i < MAX_CORES; i++) {
        init_history_buffer(&system_history.core_usage[i]);
    }
    
    for (int i = 0; i < 16; i++) {
        init_history_buffer(&system_history.storage_temps[i]);
    }
    
    system_history.storage_count = 0;
}

/**
 * Reads temperature from a file and converts from millidegrees to degrees Celsius
 * Returns -1.0 if file cannot be read or parsed
 */
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

/**
 * Reads and returns the system load averages from /proc/loadavg
 * Returns structure with -1.0 values if unable to read
 */
LoadAverage get_load_average() {
    LoadAverage load = {-1.0, -1.0, -1.0};
    FILE *file = fopen("/proc/loadavg", "r");
    if (file == NULL) {
        printf("Error: could not open /proc/loadavg\n");
        return load;
    }
    
    if (fscanf(file, "%f %f %f", &load.load_1min, &load.load_5min, &load.load_15min) != 3) {
        fclose(file);
        printf("Error: could not parse load averages\n");
        load.load_1min = load.load_5min = load.load_15min = -1.0;
        return load;
    }
    
    fclose(file);

    // Display the data directly here
    printf("System Load Average:\n");
    printf("---------------------\n");
    printf("  1 minute : %.2f\n", load.load_1min);
    printf("  5 minutes: %.2f\n", load.load_5min);
    printf(" 15 minutes: %.2f\n", load.load_15min);
    printf("---------------------\n");

    return load;
}

/**
 * Counts the number of CPU cores by parsing /proc/stat
 * Returns -1 on error, excludes the aggregate 'cpu' line
 */
int get_core_count() {
    FILE *file = fopen("/proc/stat", "r");
    if (!file) {
        printf("Error: could not open /proc/stat\n");
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

    if (count < 1) {
        printf("Error: could not determine CPU core count\n");
        return -1;
    }

    // Display the result
    printf("\nCPU Core Information:\n");
    printf("---------------------\n");
    printf(" Total cores: %d\n", count);
    printf("---------------------\n");

    return count;
}

/**
 * Reads CPU statistics for all cores from /proc/stat
 * Stores current statistics and preserves previous for delta calculations
 */
void read_cpu_stats() {
    FILE *file = fopen("/proc/stat", "r");
    if (!file) {
        perror("fopen");
        return;
    }

    char line[256];

    while (fgets(line, sizeof(line), file)) {
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
                printf("%s: user=%lu nice=%lu system=%lu idle=%lu iowait=%lu irq=%lu softirq=%lu steal=%lu\n",
                       cpu_label, stats.user, stats.nice, stats.system, stats.idle,
                       stats.iowait, stats.irq, stats.softirq, stats.steal);
            }
        }
    }

    fclose(file);
}

/**
 * Calculates CPU usage percentages for all cores based on delta between readings
 * Uses the formula: usage = (total_time - idle_time) / total_time * 100%
 */
void calculate_cpu_usage() {
    static int first_run = 1;
    static unsigned long prev_user = 0, prev_nice = 0, prev_system = 0;
    static unsigned long prev_idle = 0, prev_iowait = 0, prev_irq = 0;
    static unsigned long prev_softirq = 0, prev_steal = 0;

    for (int i = 0; i < 2; i++) {
        if (!first_run) {
            sleep(1);
        }

        FILE *file = fopen("/proc/stat", "r");
        if (!file) {
            printf("Error: Cannot open /proc/stat\n");
            return;
        }

        char line[256];
        unsigned long user, nice, system, idle, iowait, irq, softirq, steal;
        
        if (fgets(line, sizeof(line), file)) {
            if (sscanf(line, "cpu %lu %lu %lu %lu %lu %lu %lu %lu",
                      &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) < 4) {
                fclose(file);
                printf("Error: Cannot parse /proc/stat\n");
                return;
            }
        }
        fclose(file);

        if (first_run) {
            prev_user = user;
            prev_nice = nice;
            prev_system = system;
            prev_idle = idle;
            prev_iowait = iowait;
            prev_irq = irq;
            prev_softirq = softirq;
            prev_steal = steal;
            first_run = 0;
            printf("CPU Usage: 0.00%% (first measurement)\n");
            continue;
        }

        unsigned long total_diff = (user - prev_user) + (nice - prev_nice) + 
                                  (system - prev_system) + (idle - prev_idle) +
                                  (iowait - prev_iowait) + (irq - prev_irq) +
                                  (softirq - prev_softirq) + (steal - prev_steal);

        unsigned long idle_diff = (idle - prev_idle) + (iowait - prev_iowait);

        prev_user = user;
        prev_nice = nice;
        prev_system = system;
        prev_idle = idle;
        prev_iowait = iowait;
        prev_irq = irq;
        prev_softirq = softirq;
        prev_steal = steal;

        if (total_diff > 0) {
            float usage = 100.0 * (total_diff - idle_diff) / total_diff;
            
            if (usage < 0.0) usage = 0.0;
            if (usage > 100.0) usage = 100.0;
            
            printf("CPU Usage: %.2f%%\n", usage);
        }
    }
}

/**
 * Attempts to read CPU temperature from various known thermal zone locations
 * Returns -1.0 if no temperature sensor can be found
 */
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
            printf("CPU Temperature: %.2f°C\n", temp);  // assuming millidegrees
            return temp;
        }
    }
    
    printf("CPU Temperature: Not available\n");
    return -1.0;
}

/**
 * Attempts to read GPU temperature from various known graphics card locations
 * Returns -1.0 if no GPU temperature sensor can be found
 */
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
            printf("GPU Temperature: %.2f°C\n", temp);
            return temp;
        }
    }

    printf("GPU Temperature: Not available\n");
    return -1.0;
}

/**
 * Attempts to read VRM (Voltage Regulator Module) temperature from known locations
 * Returns -1.0 if no VRM temperature sensor can be found
 */
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
            printf("VRM Temperature: %.2f°C\n", temp);
            return temp;
        }
    }

    printf("VRM Temperature: Not available\n");
    return -1.0;
}

/**
 * Attempts to read chipset temperature from various known locations
 * Returns -1.0 if no chipset temperature sensor can be found
 */
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
            printf("Chipset Temperature: %.2f°C\n", temp);
            return temp;
        }
    }

    printf("Chipset Temperature: Not avaliable");
    return -1.0;
}

/**
 * Attempts to read motherboard temperature from various known locations
 * Returns -1.0 if no motherboard temperature sensor can be found
 */
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
            printf("Motherboard Temperature: %.2f°C\n", temp);
            return temp;
        }
    }

    printf("Motherboard Temperature: Not avaliable");
    return -1.0;
}

/**
 * Attempts to read PSU (Power Supply Unit) temperature from various locations
 * Searches through hwmon devices and power supply directories
 * Returns -1.0 if no PSU temperature sensor can be found
 */
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
                            printf("PSU Temperature: %.2f°C\n", temp);
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
                    printf("PSU Temperature: %.2f°C\n", temp);
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
    
    printf("PSU Temperature: Not available\n");
    return -1.0;
}

/**
 * Attempts to read case/ambient temperature from various known locations
 * Searches for sensors labeled as case, ambient, or similar
 * Returns -1.0 if no case temperature sensor can be found
 */
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
                                printf("Case Temperature: %.2f°C\n", temp);
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
                    printf("Case Temperature: %.2f°C\n", temp);
                    globfree(&glob_result);
                    return temp;
                }
            }
            globfree(&glob_result);
        }
    }

    printf("Case Temperature: Not available\n");
    return -1.0;
}

/**
 * Scans /sys/class/hwmon for storage devices (NVMe, SATA, SSD)
 * Populates the global storage_devices array with found devices
 */
void find_storage_devices_with_temperature_reporting() {
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

                            float temp_val = read_temperature_file(temp_path);
                            if (temp_val >= 0) {
                                printf("Storage Device Name: %s Temperature: %.2f°C\n", base, temp_val / 1000.0);
                            } else {
                                printf("Storage Device Name: %s Temperature: Not available\n", base);
                            }

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

// Read total CPU jiffies from /proc/stat
unsigned long long get_total_cpu_time() {
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) {
        printf("Error: Cannot open /proc/stat\n");
        return 0;
    }

    char buffer[1024];
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    
    // Read the first line which contains aggregate CPU statistics
    if (fgets(buffer, sizeof(buffer), fp)) {
        int matched = sscanf(buffer, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
               &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
        
        if (matched >= 4) {
            // Calculate total CPU time
            unsigned long long total_time = user + nice + system + idle + iowait + irq + softirq + steal;
            
            // Display the detailed breakdown
            printf("\n=== CPU Time Statistics ===\n");
            printf("Component Breakdown (in jiffies):\n");
            printf("----------------------------\n");
            printf("User mode:      %llu\n", user);
            printf("Nice mode:      %llu\n", nice);
            printf("System mode:    %llu\n", system);
            printf("Idle time:      %llu\n", idle);
            
            if (matched >= 5) printf("I/O wait:       %llu\n", iowait);
            if (matched >= 6) printf("IRQ time:       %llu\n", irq);
            if (matched >= 7) printf("Soft IRQ:       %llu\n", softirq);
            if (matched >= 8) printf("Steal time:     %llu\n", steal);
            
            printf("----------------------------\n");
            printf("Total CPU time: %llu jiffies\n", total_time);
            printf("============================\n\n");
            
            fclose(fp);
            return total_time;
        } else {
            printf("Error: Failed to parse /proc/stat. Only %d fields matched.\n", matched);
            fclose(fp);
            return 0;
        }
    } else {
        printf("Error: Cannot read from /proc/stat\n");
        fclose(fp);
        return 0;
    }
}

/**
 * Counts the number of running processes by scanning /proc directory
 * Returns -1 if /proc cannot be accessed
 */
int display_running_processes() {
    DIR *dir = opendir("/proc");
    if (!dir) {
        printf("Error: Cannot open /proc directory\n");
        return -1;
    }

    struct ProcInfo {
        char pid[32];
        char ppid[32];
        char name[256];
        char state[32];
        long ram_kb;
        double ram_percent;
        double cpu_percent;
        int level;
        int file_count;
        int socket_count;
        char network_connections[1024]; // Store brief network info
    } processes[4096];

    int proc_count = 0;
    struct dirent *entry;
    memset(processes, 0, sizeof(processes));

    // Read total system memory
    long total_mem_kb = 0;
    FILE *meminfo = fopen("/proc/meminfo", "r");
    if(meminfo) {
        char line[256];
        while(fgets(line, sizeof(line), meminfo)) {
            if(strncmp(line, "MemTotal:", 9) == 0) {
                sscanf(line + 9, "%ld", &total_mem_kb);
                break;
            }
        }
        fclose(meminfo);
    }

    // Read all numeric directories in /proc
    while ((entry = readdir(dir)) != NULL && proc_count < 4096) {
        if(entry->d_name[0] >= '0' && entry->d_name[0] <= '9') {
            char path[512];
            FILE *fp;

            // Initialize process
            strncpy(processes[proc_count].pid, entry->d_name, sizeof(processes[proc_count].pid)-1);
            strcpy(processes[proc_count].ppid, "0");
            strcpy(processes[proc_count].name, "Unknown");
            strcpy(processes[proc_count].state, "Unknown");
            processes[proc_count].ram_kb = 0;
            processes[proc_count].ram_percent = 0.0;
            processes[proc_count].cpu_percent = 0.0;
            processes[proc_count].level = -1;
            processes[proc_count].file_count = 0;
            processes[proc_count].socket_count = 0;
            strcpy(processes[proc_count].network_connections, "");

            // Read /proc/[pid]/status
            snprintf(path, sizeof(path), "/proc/%s/status", entry->d_name);
            fp = fopen(path, "r");
            if(fp) {
                char buffer[1024];
                while(fgets(buffer, sizeof(buffer), fp)) {
                    if(strncmp(buffer, "Name:", 5) == 0) {
                        char *name_start = buffer + 5;
                        while(*name_start == ' ' || *name_start == '\t') name_start++;
                        strncpy(processes[proc_count].name, name_start, sizeof(processes[proc_count].name)-1);
                        processes[proc_count].name[strcspn(processes[proc_count].name, "\n")] = 0;
                    } else if(strncmp(buffer, "State:", 6) == 0) {
                        char *state_start = buffer + 6;
                        while(*state_start == ' ' || *state_start == '\t') state_start++;
                        strncpy(processes[proc_count].state, state_start, sizeof(processes[proc_count].state)-1);
                        processes[proc_count].state[strcspn(processes[proc_count].state, "\n")] = 0;
                    } else if(strncmp(buffer, "Pid:", 4) == 0) {
                        char *pid_start = buffer + 4;
                        while(*pid_start == ' ' || *pid_start == '\t') pid_start++;
                        strncpy(processes[proc_count].pid, pid_start, sizeof(processes[proc_count].pid)-1);
                        processes[proc_count].pid[strcspn(processes[proc_count].pid, "\n")] = 0;
                    } else if(strncmp(buffer, "PPid:", 5) == 0) {
                        char *ppid_start = buffer + 5;
                        while(*ppid_start == ' ' || *ppid_start == '\t') ppid_start++;
                        strncpy(processes[proc_count].ppid, ppid_start, sizeof(processes[proc_count].ppid)-1);
                        processes[proc_count].ppid[strcspn(processes[proc_count].ppid, "\n")] = 0;
                    } else if(strncmp(buffer, "VmRSS:", 6) == 0) {
                        char *rss_start = buffer + 6;
                        while(*rss_start == ' ' || *rss_start == '\t') rss_start++;
                        sscanf(rss_start, "%ld", &processes[proc_count].ram_kb);
                    }
                }
                fclose(fp);
            }

            // Calculate RAM percentage
            if(total_mem_kb > 0)
                processes[proc_count].ram_percent = (double)processes[proc_count].ram_kb / total_mem_kb * 100.0;

            // Calculate CPU percentage
            snprintf(path, sizeof(path), "/proc/%s/stat", processes[proc_count].pid);
            fp = fopen(path, "r");
            if(fp) {
                char buffer[1024];
                if(fgets(buffer, sizeof(buffer), fp)) {
                    long utime, stime, cutime, cstime, starttime;
                    sscanf(buffer,
                        "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %ld %ld %ld %ld %*d %*d %*d %*d %*d %ld",
                        &utime, &stime, &cutime, &cstime, &starttime);
                    long total_time = utime + stime + cutime + cstime;

                    FILE *uptime_fp = fopen("/proc/uptime", "r");
                    double uptime = 0.0;
                    if(uptime_fp) {
                        fscanf(uptime_fp, "%lf", &uptime);
                        fclose(uptime_fp);
                    }

                    long clk_tck = sysconf(_SC_CLK_TCK);
                    if(uptime > 0)
                        processes[proc_count].cpu_percent = 100.0 * ((double)total_time / clk_tck) / uptime;
                }
                fclose(fp);
            }

            // Count open files and sockets
            snprintf(path, sizeof(path), "/proc/%s/fd", processes[proc_count].pid);
            DIR *fd_dir = opendir(path);
            if(fd_dir) {
                struct dirent *fd_entry;
                char fd_path[512];
                char link_target[1024];
                
                while((fd_entry = readdir(fd_dir)) != NULL) {
                    if(fd_entry->d_name[0] == '.') continue;
                    
                    processes[proc_count].file_count++;
                    
                    // Check if it's a socket
                    snprintf(fd_path, sizeof(fd_path), "/proc/%s/fd/%s", 
                             processes[proc_count].pid, fd_entry->d_name);
                    
                    ssize_t len = readlink(fd_path, link_target, sizeof(link_target)-1);
                    if(len != -1) {
                        link_target[len] = '\0';
                        if(strncmp(link_target, "socket:", 7) == 0) {
                            processes[proc_count].socket_count++;
                        }
                    }
                }
                closedir(fd_dir);
            }

            // Get network connection details from /proc/net/tcp and /proc/net/udp
            processes[proc_count].file_count = processes[proc_count].file_count;
            proc_count++;
        }
    }

    closedir(dir);

    // Calculate hierarchy levels iteratively
    int changed;
    do {
        changed = 0;
        for(int i=0;i<proc_count;i++){
            if(processes[i].level == -1){
                if(strcmp(processes[i].ppid,"0")==0){
                    processes[i].level = 0;
                    changed = 1;
                } else {
                    for(int j=0;j<proc_count;j++){
                        if(strcmp(processes[j].pid,processes[i].ppid)==0 && processes[j].level!=-1){
                            processes[i].level = processes[j].level + 1;
                            changed = 1;
                            break;
                        }
                    }
                }
            }
        }
    } while(changed);

    // Set any orphaned processes to root
    for(int i=0;i<proc_count;i++){
        if(processes[i].level == -1) processes[i].level = 0;
    }

    // Bubble sort by level then PID
    for(int i=0;i<proc_count-1;i++){
        for(int j=0;j<proc_count-i-1;j++){
            if(processes[j].level>processes[j+1].level || 
               (processes[j].level==processes[j+1].level && atoi(processes[j].pid)>atoi(processes[j+1].pid))){
                struct ProcInfo temp = processes[j];
                processes[j] = processes[j+1];
                processes[j+1] = temp;
            }
        }
    }

    // Display header
    printf("\nPROCESS TREE HIERARCHY WITH FILE/SOCKET INFO:\n");
    printf("PID (PPID)  CPU%%    RAM%%     RAM(KB)   FILES  SOCKS  STATE     COMMAND\n");
    printf("-------------------------------------------------------------------------\n");

    // Display tree
    for(int i=0;i<proc_count;i++){
        for(int j=0;j<processes[i].level;j++){
            if(j == processes[i].level - 1) printf("└── ");
            else printf("    ");
        }
        printf("%-5s (%-5s) %6.2f%% %6.2f%% %9ld %6d %6d %-8s %s\n",
            processes[i].pid,
            processes[i].ppid,
            processes[i].cpu_percent,
            processes[i].ram_percent,
            processes[i].ram_kb,
            processes[i].file_count,
            processes[i].socket_count,
            processes[i].state,
            processes[i].name);
    }

    printf("\nTotal processes: %d\n", proc_count);
    
    printf("\n=== DETAILED FILE AND NETWORK INFO FOR HIGH-RESOURCE PROCESSES ===\n");
    for(int i=0;i<proc_count;i++){
            
            printf("\n--- PID %s: %s (CPU: %.2f%%, RAM: %.2f%%, Files: %d, Sockets: %d) ---\n",
                   processes[i].pid, processes[i].name, 
                   processes[i].cpu_percent, processes[i].ram_percent,
                   processes[i].file_count, processes[i].socket_count);
            
            // Show open files (first 10)
            char path[512];
            snprintf(path, sizeof(path), "/proc/%s/fd", processes[i].pid);
            DIR *fd_dir = opendir(path);
            if(fd_dir) {
                printf("Open files (first 10):\n");
                struct dirent *fd_entry;
                char fd_path[512];
                char link_target[1024];
                int file_count = 0;
                
                while((fd_entry = readdir(fd_dir)) != NULL) {
                    if(fd_entry->d_name[0] == '.') continue;
                    
                    snprintf(fd_path, sizeof(fd_path), "/proc/%s/fd/%s", 
                             processes[i].pid, fd_entry->d_name);
                    
                    ssize_t len = readlink(fd_path, link_target, sizeof(link_target)-1);
                    if(len != -1) {
                        link_target[len] = '\0';
                        printf("  FD %s -> %s\n", fd_entry->d_name, link_target);
                        file_count++;
                    }
                }
                closedir(fd_dir);
            }
            
            // Show network connections from /proc/net/tcp
            printf("Network connections:\n");
            FILE *tcp_file = fopen("/proc/net/tcp", "r");
            if(tcp_file) {
                char line[1024];
                fgets(line, sizeof(line), tcp_file); // Skip header
                while(fgets(line, sizeof(line), tcp_file)) {
                    // Basic parsing - you can enhance this to show more details
                    char local_addr[64], remote_addr[64], state[16];
                    int uid, inode;
                    if(sscanf(line, "%*d: %63s %63s %15s %*x:%*x %*x:%*x %*x %d %*d %d",
                             local_addr, remote_addr, state, &uid, &inode) >= 5) {
                        
                        // Check if this inode matches any of the process's socket inodes
                        char socket_path[64];
                        snprintf(socket_path, sizeof(socket_path), "socket:[%d]", inode);
                        
                        // Check if this socket belongs to our process
                        snprintf(path, sizeof(path), "/proc/%s/fd", processes[i].pid);
                        DIR *check_dir = opendir(path);
                        if(check_dir) {
                            struct dirent *check_entry;
                            char check_path[512];
                            char check_link[1024];
                            
                            while((check_entry = readdir(check_dir)) != NULL) {
                                if(check_entry->d_name[0] == '.') continue;
                                
                                snprintf(check_path, sizeof(check_path), "/proc/%s/fd/%s", 
                                         processes[i].pid, check_entry->d_name);
                                
                                ssize_t len = readlink(check_path, check_link, sizeof(check_link)-1);
                                if(len != -1) {
                                    check_link[len] = '\0';
                                    if(strcmp(check_link, socket_path) == 0) {
                                        printf("  TCP %s -> %s (%s)\n", local_addr, remote_addr, state);
                                        break;
                                    }
                                }
                            }
                            closedir(check_dir);
                        }
                    }
                }
                fclose(tcp_file);
            }
    }
    
    return proc_count;
}

#define CMD_BUFFER_SIZE 1024

/**
 * Find the full path of the smartctl binary for querying S.M.A.R.T. data from drives
 */
const char *find_smartctl_path() {
    FILE *pipe;
    char path_buffer[256];
    static char smartctl_path[256];
    
    pipe = popen("which smartctl 2>/dev/null", "r");
    if (pipe != NULL) {
        if (fgets(path_buffer, sizeof(path_buffer), pipe) != NULL) {
            path_buffer[strcspn(path_buffer, "\n")] = 0;
            pclose(pipe);
            if (access(path_buffer, X_OK) == 0) {
                strncpy(smartctl_path, path_buffer, sizeof(smartctl_path));
                return smartctl_path;
            }
        }
        pclose(pipe);
    }
    
    const char *common_paths[] = {
        "/usr/sbin/smartctl",
        "/usr/bin/smartctl",
        "/sbin/smartctl",
        "/usr/local/sbin/smartctl",
        "/usr/local/bin/smartctl",
        NULL
    };
    
    for (int i = 0; common_paths[i] != NULL; i++) {
        if (access(common_paths[i], X_OK) == 0) {
            return common_paths[i];
        }
    }
    
    return NULL;
}

/**
 * Detect all storage devices on the system
 */
void detect_all_storage_devices() {
    const char *patterns[] = {
        "/dev/sd*", "/dev/nvme*n*", "/dev/mmcblk*", "/dev/vd*", "/dev/hd*", NULL
    };

    for (int i = 0; patterns[i] != NULL; i++) {
        glob_t glob_result;
        if (glob(patterns[i], GLOB_MARK, NULL, &glob_result) != 0) continue;

        for (size_t j = 0; j < glob_result.gl_pathc; j++) {
            char *path = glob_result.gl_pathv[j];

            // skip partitions (like nvme0n1p1) and non-numeric devices
            char *base = strrchr(path, '/');
            if (!base) base = path; else base++;
            if (strchr(base, 'p') || strspn(base, "0123456789") != 0) continue;

            struct stat st;
            if (stat(path, &st) != 0 || !S_ISBLK(st.st_mode)) continue;

            printf("Detected storage device: %s\n", path);
        }
        globfree(&glob_result);
    }
}

/**
 * Run SmartCTL and print its report
 */
void print_smart_data() {
    const char *smartctl_path = "/usr/sbin/smartctl";
    const char *patterns[] = { "/dev/sd*", "/dev/nvme*n*", "/dev/mmcblk*", "/dev/vd*", "/dev/hd*", NULL };

    // Check if smartctl exists
    struct stat st;
    if (stat(smartctl_path, &st) != 0 || !S_ISREG(st.st_mode)) {
        fprintf(stderr, "smartctl not found at %s\n", smartctl_path);
        return;
    }

    for (int i = 0; patterns[i] != NULL; i++) {
        glob_t glob_result;
        if (glob(patterns[i], GLOB_MARK, NULL, &glob_result) != 0) continue;

        for (size_t j = 0; j < glob_result.gl_pathc; j++) {
            char *device = glob_result.gl_pathv[j];

            // Skip partitions like /dev/sda1 or nvme0n1p1
            char *base = strrchr(device, '/');
            if (!base) base = device; else base++;
            if (strchr(base, 'p') || strspn(base, "0123456789") != 0) continue;

            // Only process block devices
            if (stat(device, &st) != 0 || !S_ISBLK(st.st_mode)) continue;

            // Build command dynamically
            size_t cmd_len = strlen("sudo ") + strlen(smartctl_path) + strlen(" -a ") +
                             strlen(device) + strlen(" 2>/dev/null") + 1;
            char *command = malloc(cmd_len);
            if (!command) {
                perror("malloc failed");
                continue;
            }

            snprintf(command, cmd_len, "sudo %s -a %s 2>/dev/null", smartctl_path, device);

            printf("\n === S.M.A.R.T. Data for %s ===\n", device);
            printf("Executing: %s\n\n", command);

            FILE *pipe = popen(command, "r");
            if (!pipe) {
                perror("popen failed");
                free(command);
                continue;
            }

            char buffer[256];
            while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
                printf("%s", buffer);
            }

            pclose(pipe);
            free(command);
        }

        globfree(&glob_result);
    }
}

void print_uname_info() {
    struct utsname info;
    if (uname(&info) == 0) {
        printf("\n=== System Information (uname) ===\n");
        printf("OS: %s\n", info.sysname);
        printf("Hostname: %s\n", info.nodename);
        printf("Kernel Release: %s\n", info.release);
        printf("Kernel Version: %s\n", info.version);
        printf("Architecture: %s\n", info.machine);
    } else {
        perror("uname failed");
    }
}

void print_detailed_os_info() {
    printf("\n=== Detailed OS Information ===\n");
    
    const char *release_files[] = {
        "/etc/os-release",
        "/usr/lib/os-release",
        "/etc/lsb-release",
        "/etc/debian_version",
        "/etc/redhat-release",
        "/etc/centos-release",
        "/etc/fedora-release",
        "/etc/SuSE-release",
        "/etc/arch-release",
        NULL
    };
    
    for (int i = 0; release_files[i] != NULL; i++) {
        FILE *fp = fopen(release_files[i], "r");
        if (fp) {
            printf("--- %s ---\n", release_files[i]);
            char line[256];
            while (fgets(line, sizeof(line), fp)) {
                printf("%s", line);
            }
            fclose(fp);
            printf("\n");
        }
    }
}

void print_kernel_details() {
    printf("\n=== Kernel Details ===\n");
    
    FILE *fp = fopen("/proc/version", "r");
    if (fp) {
        char version[256];
        if (fgets(version, sizeof(version), fp)) {
            printf("Full Kernel Version: %s", version);
        }
        fclose(fp);
    }
    
    fp = fopen("/proc/version_signature", "r");
    if (fp) {
        char sig[256];
        if (fgets(sig, sizeof(sig), fp)) {
            printf("Kernel Signature: %s", sig);
        }
        fclose(fp);
    }
    
    fp = fopen("/proc/cmdline", "r");
    if (fp) {
        char cmdline[1024];
        if (fgets(cmdline, sizeof(cmdline), fp)) {
            printf("Kernel Command Line: %s\n", cmdline);
        }
        fclose(fp);
    }
    
    printf("Kernel Architecture: ");
    #if defined(__x86_64__)
    printf("x86_64\n");
    #elif defined(__i386__)
    printf("i386\n");
    #elif defined(__aarch64__)
    printf("ARM64\n");
    #elif defined(__arm__)
    printf("ARM\n");
    #elif defined(__powerpc64__)
    printf("PPC64\n");
    #elif defined(__mips__)
    printf("MIPS\n");
    #else
    printf("Unknown\n");
    #endif
}

void print_distribution_info() {
    const char *package_managers[] = {
        "/etc/apt/sources.list",
        "/etc/yum.repos.d/",
        "/etc/dnf/dnf.conf",
        "/etc/zypp/zypp.conf",
        "/etc/pacman.conf",
        "/etc/portage/make.conf",
        "/etc/emerge/",
        NULL
    };
    
    const char *pm_names[] = {
        "APT (Debian/Ubuntu)",
        "YUM (RedHat/CentOS)",
        "DNF (Fedora/RHEL8+)",
        "Zypper (openSUSE)",
        "Pacman (Arch)",
        "Portage (Gentoo)",
        "Emerge (Gentoo)"
    };
    
    // Package Manager
    for (int i = 0; package_managers[i] != NULL; i++) {
        FILE *fp = fopen(package_managers[i], "r");
        if (fp) {
            printf("%s\n", pm_names[i]);
            fclose(fp);
            break;
        }
        if (strstr(package_managers[i], "/etc/yum.repos.d/") || 
            strstr(package_managers[i], "/etc/emerge/")) {
            DIR *dir = opendir(package_managers[i]);
            if (dir) {
                printf("%s\n", pm_names[i]);
                closedir(dir);
                break;
            }
        }
    }
    
    // Init System
    FILE *fp = fopen("/proc/1/comm", "r");
    if (fp) {
        char init_system[32];
        if (fgets(init_system, sizeof(init_system), fp)) {
            init_system[strcspn(init_system, "\n")] = 0;
            printf("%s\n", init_system);
        }
        fclose(fp);
    } else {
        FILE *cmd = popen("ps -p 1 -o comm= 2>/dev/null", "r");
        if (cmd) {
            char init_system[32];
            if (fgets(init_system, sizeof(init_system), cmd)) {
                init_system[strcspn(init_system, "\n")] = 0;
                printf("%s\n", init_system);
            }
            pclose(cmd);
        } else {
            char init_path[256];
            ssize_t len = readlink("/sbin/init", init_path, sizeof(init_path)-1);
            if (len != -1) {
                init_path[len] = '\0';
                char *basename = strrchr(init_path, '/');
                printf("%s\n", basename ? basename + 1 : init_path);
            } else {
                printf("Unknown\n");
            }
        }
    }

    // Distribution version
    const char *version_files[] = {
        "/etc/debian_version",
        "/etc/redhat-release",
        "/etc/centos-release", 
        "/etc/fedora-release",
        "/etc/SuSE-release",
        "/etc/arch-release",
        "/etc/slackware-version",
        "/etc/gentoo-release",
        NULL
    };
    
    for (int i = 0; version_files[i] != NULL; i++) {
        fp = fopen(version_files[i], "r");
        if (fp) {
            char version[128];
            if (fgets(version, sizeof(version), fp)) {
                version[strcspn(version, "\n")] = 0;
                printf("%s\n", version);
            }
            fclose(fp);
            break;
        }
    }
    
    // Systemd version
    fp = fopen("/proc/1/comm", "r");
    if (fp) {
        char init_system[32];
        if (fgets(init_system, sizeof(init_system), fp)) {
            init_system[strcspn(init_system, "\n")] = 0;
            if (strcmp(init_system, "systemd") == 0) {
                FILE *cmd = popen("systemctl --version 2>/dev/null | head -1 | awk '{print $2}'", "r");
                if (cmd) {
                    char systemd_version[64];
                    if (fgets(systemd_version, sizeof(systemd_version), cmd)) {
                        systemd_version[strcspn(systemd_version, "\n")] = 0;
                        printf("%s\n", systemd_version);
                    }
                    pclose(cmd);
                }
            }
        }
        fclose(fp);
    }
}

void print_library_versions() {
    printf("\n=== Library Versions ===\n");
    
    printf("GLIBC Version: %s\n", gnu_get_libc_version());
    printf("GLIBC Release: %s\n", gnu_get_libc_release());
    
    #ifdef __GLIBC__
    printf("Using GLIBC: %d.%d\n", __GLIBC__, __GLIBC_MINOR__);
    #endif
    
    #ifdef __GNUC__
    printf("GCC Version: %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
    #endif
    
    #ifdef __STDC_VERSION__
    printf("C Standard: %ld\n", __STDC_VERSION__);
    #endif
}

void print_security_info() {
    printf("\n=== Security & Update Information ===\n");
    
    FILE *fp = fopen("/etc/apt/sources.list", "r");
    if (fp) {
        char line[256];
        int security_updates = 0;
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "security") || strstr(line, "updates")) {
                security_updates = 1;
                break;
            }
        }
        fclose(fp);
        printf("Security updates configured: %s\n", security_updates ? "Yes" : "No");
    }
    
    fp = fopen("/var/lib/apt/periodic/update-success-stamp", "r");
    if (fp) {
        char timestamp[64];
        if (fgets(timestamp, sizeof(timestamp), fp)) {
            printf("Last successful update: %s", timestamp);
        }
        fclose(fp);
    }
    
    fp = fopen("/sys/kernel/security/lsm", "r");
    if (fp) {
        char lsm[256];
        if (fgets(lsm, sizeof(lsm), fp)) {
            printf("Security Modules: %s", lsm);
        }
        fclose(fp);
    }
    printf("\n");
}

void print_system_limits() {
    printf("=== System Limits ===\n");
    
    FILE *fp = fopen("/proc/sys/kernel/pid_max", "r");
    if (fp) {
        char pid_max[32];
        if (fgets(pid_max, sizeof(pid_max), fp)) {
            printf("Maximum PID: %s", pid_max);
        }
        fclose(fp);
    }
    
    fp = fopen("/proc/sys/kernel/threads-max", "r");
    if (fp) {
        char threads_max[32];
        if (fgets(threads_max, sizeof(threads_max), fp)) {
            printf("Maximum threads: %s", threads_max);
        }
        fclose(fp);
    }
    
    fp = fopen("/proc/sys/kernel/pty/max", "r");
    if (fp) {
        char pty_max[32];
        if (fgets(pty_max, sizeof(pty_max), fp)) {
            printf("Maximum PTYs: %s", pty_max);
        }
        fclose(fp);
    }
}

// Note: To run this function, run the program without root privileges
void check_startup_directories() {
    printf("\n=== STARTUP APPLICATIONS ===\n");
    
    const char *dirs[] = {
        "~/.config/autostart",
        "~/.config/autostart-scripts",
        "/etc/xdg/autostart",
        "~/.kde/Autostart",
        "~/.local/share/autostart",
        NULL
    };
    
    char expanded_path[512];
    char *home = getenv("HOME");

    for (int i = 0; dirs[i] != NULL; i++) {
        // Expand ~ to home directory
        if (dirs[i][0] == '~' && home != NULL) {
            snprintf(expanded_path, sizeof(expanded_path), "%s%s", home, dirs[i] + 1);
        } else {
            strncpy(expanded_path, dirs[i], sizeof(expanded_path));
            expanded_path[sizeof(expanded_path)-1] = '\0';
        }

        printf("\nChecking: %s\n", expanded_path);

        DIR *dir = opendir(expanded_path);
        if (!dir) {
            printf("  Directory not found or inaccessible\n");
            continue;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            // Include regular files and symlinks (for .desktop files)
            if (entry->d_type == DT_REG || entry->d_type == DT_LNK) {
                const char *ext = strrchr(entry->d_name, '.');
                if (ext && strcmp(ext, ".desktop") == 0) {
                    printf("  %s\n", entry->d_name);
                }
            }
        }
        closedir(dir);
    }
}

// Note: To run this function, run the program without root privileges
void check_systemd_user_services() {
    printf("\n=== SYSTEMD USER SERVICES (STARTUP) ===\n");
    system("systemctl --user list-unit-files --type=service --state=enabled | grep -E '(enabled|autostart)' | head -10");
}

void detect_all_package_managers() {
    const char *managers[] = {
        "apt",
        "yum",
        "dnf",
        "pacman",
        "zypper",
        "brew",
        "choco",
        "winget"
    };

    const char *version_flags[] = {
        "--version",
        "--version",
        "--version",
        "--version",
        "--version",
        "--version",
        "--version",
        "--version"
    };

    const char *list_flags[] = {
        "list --installed", // apt
        "list installed",   // yum
        "list installed",   // dnf
        "-Q",               // pacman
        "se -i",            // zypper
        "list",             // brew
        "list -l",          // choco
        "list"              // winget
    };

    char command[512];
    char buffer[512];
    FILE *fp;
    int found_any = 0;

    printf("Detecting package managers and listing installed packages:\n");

    for (int i = 0; i < sizeof(managers)/sizeof(managers[0]); i++) {
        // Check version
        snprintf(command, sizeof(command), "%s %s 2>&1", managers[i], version_flags[i]);
        fp = popen(command, "r");
        if (fp) {
            if (fgets(buffer, sizeof(buffer), fp) != NULL) {
                buffer[strcspn(buffer, "\n")] = 0;
                printf("\n%s detected: %s\n", managers[i], buffer);
                found_any = 1;
            }
            pclose(fp);

            // List installed packages
            snprintf(command, sizeof(command), "%s %s 2>&1", managers[i], list_flags[i]);
            fp = popen(command, "r");
            if (fp) {
                printf("Installed packages for %s:\n", managers[i]);
                while (fgets(buffer, sizeof(buffer), fp) != NULL) {
                    buffer[strcspn(buffer, "\n")] = 0;
                    printf("  %s\n", buffer);
                }
                pclose(fp);
            }
        }
    }

    if (!found_any) {
        printf("No known package managers detected.\n");
    }
}

void scan_directory(const char *path) {
    struct dirent *entry;
    struct stat info;
    char fullpath[1024];

    DIR *dir = opendir(path);
    if (!dir) return;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
        if (stat(fullpath, &info) != 0) continue;

        if (S_ISDIR(info.st_mode)) {
            printf("[DIR] %s\n", fullpath);
            scan_directory(fullpath);
        } else {
            printf("[FILE] %s\n", fullpath);
        }
    }

    closedir(dir);
}

// List programs installed into standard directories
void list_manual_installs() {
    printf("Manually installed programs/binaries:\n");

    // Standard directories
    scan_directory("/usr/local/bin");
    scan_directory("/opt");

    // User-specific local binaries
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        if (pw) home = pw->pw_dir;
    }

    if (home) {
        char path[1024];
        snprintf(path, sizeof(path), "%s/.local/bin", home);
        scan_directory(path);
    }
}

void read_journal_logs() {
    FILE *fp;
    char buffer[1024];
    int read_error = 0;

    // Run the journalctl command
    fp = popen("journalctl --no-pager -n 300 2>&1", "r");
    if (fp == NULL) {
        printf("Error: Failed to run journalctl command\n");
        fflush(stdout);
        return;
    }

    // Read the output line by line and print it
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        printf("Journal: %s", buffer);
    }

    // Check if there was an error during reading
    if (ferror(fp)) {
        read_error = 1;
        perror("Error reading from journalctl output");
        fprintf(stderr, "Error: Failed to read data from journalctl command.\n");
    }

    // Close the stream and check for command execution errors
    int exit_status = pclose(fp);
    if (exit_status == -1) {
        perror("Failed to close journalctl process");
        fprintf(stderr, "Error: Could not properly close the journalctl command.\n");
    } else if (exit_status != 0) {
        fprintf(stderr, "Error: journalctl command failed with exit status %d\n", exit_status);
        fprintf(stderr, "This may indicate permission issues or journalctl errors.\n");
    }

    fflush(stdout);
}
// Calculate total time computer was on in different states
long get_total_jiffies() {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) {
        printf("Error: Could not open /proc/stat\n");
        return -1;
    }
    
    char cpu[16];
    long user, nice, system, idle, iowait, irq, softirq, steal;
    
    int result = fscanf(f, "%s %ld %ld %ld %ld %ld %ld %ld %ld",
                       cpu, &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
    
    fclose(f);
    
    if (result != 9) {
        printf("Error: Failed to parse /proc/stat content\n");
        return -1;
    }
    
    long total_jiffies = user + nice + system + idle + iowait + irq + softirq + steal;
    
    printf("CPU Statistics from /proc/stat:\n");
    printf("CPU: %s\n", cpu);
    printf("User: %ld\n", user);
    printf("Nice: %ld\n", nice);
    printf("System: %ld\n", system);
    printf("Idle: %ld\n", idle);
    printf("IOWait: %ld\n", iowait);
    printf("IRQ: %ld\n", irq);
    printf("SoftIRQ: %ld\n", softirq);
    printf("Steal: %ld\n", steal);
    printf("Total Jiffies: %ld\n", total_jiffies);
    printf("------------------------------\n");
    
    return total_jiffies;
}

void show_system_uptime_and_cpu_sleep_time() {
    FILE *fp = fopen("/proc/uptime", "r");
    if (!fp) {
        perror("Error opening /proc/uptime");
        return;
    }

    double uptime_seconds, sleep_seconds;
    if (fscanf(fp, "%lf %lf", &uptime_seconds, &sleep_seconds) != 2) {
        fprintf(stderr, "Error reading from /proc/uptime\n");
        fclose(fp);
        return;
    }

    fclose(fp);

    printf("System Uptime: %.2f seconds\n", uptime_seconds);
    printf("CPU Sleep Time: %.2f seconds\n", sleep_seconds);
}

#define MAX_BUFFER 1024
#define MAX_OUTPUT_SIZE 65536

typedef struct {
    char cpu_info[MAX_OUTPUT_SIZE];
    char memory_info[MAX_OUTPUT_SIZE];
    char storage_info[MAX_OUTPUT_SIZE];
    char motherboard_info[MAX_OUTPUT_SIZE];
    char bios_info[MAX_OUTPUT_SIZE];
    char pci_info[MAX_OUTPUT_SIZE];
    char usb_info[MAX_OUTPUT_SIZE];
    char network_info[MAX_OUTPUT_SIZE];
    char kernel_info[MAX_OUTPUT_SIZE];
    char system_info[MAX_OUTPUT_SIZE];
} HardwareData;

char* run_command(const char *command) {
    static char output[MAX_OUTPUT_SIZE];
    output[0] = '\0';
    
    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        snprintf(output, sizeof(output), "Command not available: %s", command);
        return output;
    }
    
    char buffer[MAX_BUFFER];
    size_t total_size = 0;
    
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        size_t buffer_len = strlen(buffer);
        if (total_size + buffer_len < MAX_OUTPUT_SIZE - 1) {
            strcat(output, buffer);
            total_size += buffer_len;
        }
    }
    
    pclose(fp);
    return output;
}

void display_section(const char *title, const char *content) {
    printf("\n\033[1;34m%s\033[0m\n", title);
    printf("\033[1;32m");
    for (int i = 0; i < strlen(title); i++) printf("=");
    printf("\033[0m\n");
    
    if (content && strlen(content) > 0) {
        printf("%s\n", content);
    } else {
        printf("Information not available\n");
    }
}

void collect_hardware_data(HardwareData *data) {
    // Initialize all buffers
    memset(data, 0, sizeof(HardwareData));
    
    // CPU Information (combine multiple sources)
    char *lscpu_output = run_command("lscpu 2>/dev/null");
    char *cpuinfo_output = run_command("cat /proc/cpuinfo | grep 'model name\\|cpu cores\\|cpu MHz' | head -10 2>/dev/null");
    snprintf(data->cpu_info, sizeof(data->cpu_info), 
             "CPU Details:\n%s\n\nProcessor Information:\n%s", 
             lscpu_output, cpuinfo_output);
    
    // Memory Information (combine multiple sources)
    char *free_output = run_command("free -h 2>/dev/null");
    char *meminfo_output = run_command("cat /proc/meminfo | head -15 2>/dev/null");
    char *dmi_memory = run_command("sudo dmidecode -t memory 2>/dev/null | head -30");
    snprintf(data->memory_info, sizeof(data->memory_info),
             "Memory Usage:\n%s\n\nMemory Details:\n%s\n\nDMI Memory Info:\n%s",
             free_output, meminfo_output, dmi_memory);
    
    // Storage Information
    char *lsblk_output = run_command("lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,FSTYPE,MODEL 2>/dev/null");
    char *df_output = run_command("df -h -T 2>/dev/null");
    snprintf(data->storage_info, sizeof(data->storage_info),
             "Block Devices:\n%s\n\nDisk Usage:\n%s",
             lsblk_output, df_output);
    
    // Motherboard Information
    char *baseboard = run_command("sudo dmidecode -t baseboard 2>/dev/null");
    char *chassis = run_command("sudo dmidecode -t chassis 2>/dev/null");
    snprintf(data->motherboard_info, sizeof(data->motherboard_info),
             "Baseboard:\n%s\n\nChassis:\n%s",
             baseboard, chassis);
    
    // BIOS Information
    char *bios = run_command("sudo dmidecode -t bios 2>/dev/null");
    snprintf(data->bios_info, sizeof(data->bios_info), "%s", bios);
    
    // PCI Devices
    char *lspci = run_command("lspci 2>/dev/null");
    snprintf(data->pci_info, sizeof(data->pci_info), "%s", lspci);
    
    // USB Devices
    char *lsusb = run_command("lsusb 2>/dev/null");
    snprintf(data->usb_info, sizeof(data->usb_info), "%s", lsusb);
    
    // Network Information
    char *network = run_command("ip link show 2>/dev/null");
    snprintf(data->network_info, sizeof(data->network_info), "%s", network);
    
    // Kernel Information
    char *lsmod = run_command("lsmod | head -20 2>/dev/null");
    snprintf(data->kernel_info, sizeof(data->kernel_info), "%s", lsmod);
    
    // System Overview
    char *lshw = run_command("sudo lshw -short 2>/dev/null | head -20");
    char *hostname = run_command("hostnamectl 2>/dev/null");
    snprintf(data->system_info, sizeof(data->system_info),
             "System Overview:\n%s\n\nHost Information:\n%s",
             lshw, hostname);
}

void display_hardware_info() {
    HardwareData data;
    collect_hardware_data(&data);
    
    printf("\033[1;35m");
    printf("================================================================================\n");
    printf("                         SYSTEM HARDWARE INFORMATION\n");
    printf("================================================================================\n");
    printf("\033[0m");
    
    // System Overview
    display_section("SYSTEM OVERVIEW", data.system_info);
    
    // Processor Section
    display_section("PROCESSOR INFORMATION", data.cpu_info);
    
    // Memory Section
    display_section("MEMORY INFORMATION", data.memory_info);
    
    // Storage Section
    display_section("STORAGE DEVICES", data.storage_info);
    
    // Motherboard & BIOS Section
    char mobo_bios[MAX_OUTPUT_SIZE];
    snprintf(mobo_bios, sizeof(mobo_bios), 
             "Motherboard Information:\n%s\n\nBIOS Information:\n%s",
             data.motherboard_info, data.bios_info);
    display_section("MOTHERBOARD & BIOS", mobo_bios);
    
    // Hardware Devices Section
    char devices_info[MAX_OUTPUT_SIZE];
    snprintf(devices_info, sizeof(devices_info),
             "PCI Devices:\n%s\n\nUSB Devices:\n%s\n\nNetwork Interfaces:\n%s",
             data.pci_info, data.usb_info, data.network_info);
    display_section("HARDWARE DEVICES", devices_info);
    
    // Kernel Section
    display_section("KERNEL INFORMATION", data.kernel_info);
    
    printf("\033[1;35m");
    printf("================================================================================\n");
    printf("                         END OF SYSTEM INFORMATION\n");
    printf("================================================================================\n");
    printf("\033[0m");
}

void monitor_cpu_utilization() {
    CPUUtilization prev, curr;
    
    printf("Real-time CPU utilization monitoring\n");
    
    printf("%-20s %-12s %-12s %-12s %-12s\n", 
           "Timestamp", "User%", "System%", "IOWait%", "Total%");
    printf("-------------------------------------------------------------\n");
    
    // First reading
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) {
        perror("Error opening /proc/stat");
        return;
    }
    
    char line[256];
    if (fgets(line, sizeof(line), fp)) {
        sscanf(line, "cpu %lu %lu %lu %lu %lu %lu %lu %lu",
               &prev.user, &prev.nice, &prev.system, &prev.idle,
               &prev.iowait, &prev.irq, &prev.softirq, &prev.steal);
    }
    fclose(fp);
    
    sleep(1);
    
    while (1) {
        // Get current timestamp
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char timestamp[20];
        strftime(timestamp, sizeof(timestamp), "%H:%M:%S", tm_info);
        
        // Second reading
        fp = fopen("/proc/stat", "r");
        if (!fp) {
            perror("Error opening /proc/stat");
            return;
        }
        
        if (fgets(line, sizeof(line), fp)) {
            sscanf(line, "cpu %lu %lu %lu %lu %lu %lu %lu %lu",
                   &curr.user, &curr.nice, &curr.system, &curr.idle,
                   &curr.iowait, &curr.irq, &curr.softirq, &curr.steal);
        }
        fclose(fp);
        
        // Calculate total idle time (idle + iowait)
        unsigned long prev_idle = prev.idle + prev.iowait;
        unsigned long curr_idle = curr.idle + curr.iowait;
        
        // Calculate total non-idle time
        unsigned long prev_non_idle = prev.user + prev.nice + prev.system + 
                                     prev.irq + prev.softirq + prev.steal;
        unsigned long curr_non_idle = curr.user + curr.nice + curr.system + 
                                     curr.irq + curr.softirq + curr.steal;
        
        // Calculate total time
        unsigned long prev_total = prev_idle + prev_non_idle;
        unsigned long curr_total = curr_idle + curr_non_idle;
        
        // Calculate differences
        unsigned long total_delta = curr_total - prev_total;
        unsigned long idle_delta = curr_idle - prev_idle;
        
        double total_utilization = 0.0;
        if (total_delta > 0) {
            total_utilization = ((double)(total_delta - idle_delta) / total_delta) * 100.0;
        }
        
        unsigned long total_prev = (prev.user + prev.nice + prev.system + prev.idle + 
                                  prev.iowait + prev.irq + prev.softirq + prev.steal);
        unsigned long total_curr = (curr.user + curr.nice + curr.system + curr.idle + 
                                  curr.iowait + curr.irq + curr.softirq + curr.steal);
        unsigned long total_component_delta = total_curr - total_prev;
        
        double user_pct = 0.0;
        double system_pct = 0.0;
        double iowait_pct = 0.0;
        
        if (total_component_delta > 0) {
            user_pct = ((double)(curr.user - prev.user) / total_component_delta) * 100;
            system_pct = ((double)(curr.system - prev.system) / total_component_delta) * 100;
            iowait_pct = ((double)(curr.iowait - prev.iowait) / total_component_delta) * 100;
        }
        
        printf("%-20s %-12.1f %-12.1f %-12.1f %-12.1f\n", 
               timestamp, user_pct, system_pct, iowait_pct, total_utilization);
        
        fflush(stdout);
        
        prev = curr;
        
        sleep(1);
    }
}

void check_firewall() {
    FILE *fp;
    char buffer[256];
    int iptables_active = 0;
    int nftables_active = 0;
    int ufw_active = 0;
    
    printf("Firewall Status:\n");
    
    // Check iptables
    fp = popen("iptables -L -n 2>/dev/null | head -n 10 | wc -l", "r");
    if (fp != NULL) {
        if (fgets(buffer, sizeof(buffer), fp) != NULL) {
            iptables_active = (atoi(buffer) > 3);
        }
        pclose(fp);
    }
    printf("iptables: %s\n", iptables_active ? "ACTIVE" : "inactive");
    
    // Check nftables
    fp = popen("nft list ruleset 2>/dev/null | head -n 5 | wc -l", "r");
    if (fp != NULL) {
        if (fgets(buffer, sizeof(buffer), fp) != NULL) {
            nftables_active = (atoi(buffer) > 1);
        }
        pclose(fp);
    }
    printf("nftables: %s\n", nftables_active ? "ACTIVE" : "inactive");
    
    // Check UFW
    fp = popen("which ufw >/dev/null 2>&1 && ufw status 2>/dev/null | grep -q active && echo 1 || echo 0", "r");
    if (fp != NULL) {
        if (fgets(buffer, sizeof(buffer), fp) != NULL) {
            ufw_active = atoi(buffer);
        }
        pclose(fp);
    }
    printf("UFW: %s\n", ufw_active ? "ACTIVE" : "inactive");
    
    // Overall status
    int overall_status = iptables_active || nftables_active || ufw_active;
    printf("\nOverall firewall status: %s\n", overall_status ? "ACTIVE" : "INACTIVE");
}

void show_logged_in_users() {
    char* result = run_command("w");
    if (result) {
        printf("Logged in users:\n%s\n", result);
    }
    else {
        printf("Failed to get logged in users.\n");
    }
}

int view_system_logs() {
    printf("System Log Viewer\n");
    printf("=================\n\n");
    
    printf("Current user UID: %d\n", getuid());
    printf("Current user GID: %d\n\n", getgid());
    
    // Try different methods to read logs
    const char *commands[] = {
        "journalctl -n 30 --no-pager",
        "dmesg | tail -30",
        "cat /var/log/syslog 2>/dev/null | tail -20",
        "cat /var/log/messages 2>/dev/null | tail -20",
        "cat /var/log/auth.log 2>/dev/null | tail -20",
        "cat /var/log/kern.log 2>/dev/null | tail -20",
        "ls -la /var/log/ 2>/dev/null | head -10",
        NULL
    };
    
    const char *descriptions[] = {
        "System Journal (journalctl):",
        "Kernel Messages (dmesg):",
        "System Log (/var/log/syslog):",
        "System Messages (/var/log/messages):",
        "Auth Log (/var/log/auth.log):",
        "Kernel Log (/var/log/kern.log):",
        "Log Directory Contents:",
        NULL
    };
    
    for (int i = 0; commands[i] != NULL; i++) {
        printf("\n=== %s ===\n", descriptions[i]);
        printf("Command: %s\n", commands[i]);
        printf("Output:\n");
        printf("--------\n");
        
        int result = system(commands[i]);
        if (result != 0) {
            printf("Command failed or no output\n");
        }
        printf("\n");
    }
    
    return 0;
}

int main(int argc, char *argv[]) {
    // Initialize system components
    init_system_history();
    
    // If no arguments provided, show usage
    if (argc == 1) {
        printf("get_core_count\n");
        printf("calculate_cpu_usage\n");
        printf("read_cpu_stats\n");
        printf("monitor_cpu_utilization\n");
        printf("get_load_average\n");
        printf("get_cpu_temperature\n");
        printf("get_gpu_temperature\n");
        printf("get_vrm_temperature\n");
        printf("get_chipset_temperature\n");
        printf("get_motherboard_temperature\n");
        printf("get_psu_temperature\n");
        printf("get_case_temperature\n");
        printf("find_storage_devices_with_temperature_reporting\n");
        printf("detect_all_storage_devices\n");
        printf("print_smart_data\n"); // Requires sudo
        printf("display_running_processes\n");
        printf("display_hardware_info\n"); // Requires sudo
        printf("print_kernel_details\n");
        printf("print_distribution_info\n");
        printf("print_library_versions\n");
        printf("print_security_info\n");
        printf("detect_all_package_managers\n");
        printf("list_manual_installs\n");
        printf("check_startup_directories\n");
        printf("check_systemd_user_services\n");
        printf("show_system_uptime_and_cpu_sleep_time\n");
        printf("get_total_jiffies\n");
        printf("check_firewall\n");
        printf("show_logged_in_users\n");
        printf("view_system_logs\n");
        printf("read_journal_logs\n"); // Requires sudo
        printf("get_total_cpu_time\n");
        printf("print_uname_info\n");
        printf("print_detailed_os_info\n");
        printf("print_system_limits\n");
        printf("scan_directory directory_name\n");
        return 0;
    }
    
    // Handle specific function calls via command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "get_core_count") == 0) {
            get_core_count();
        }
        else if (strcmp(argv[i], "calculate_cpu_usage") == 0) {
            calculate_cpu_usage();
        }
        else if (strcmp(argv[i], "read_cpu_stats") == 0) {
            read_cpu_stats();
        }
        else if (strcmp(argv[i], "monitor_cpu_utilization") == 0) {
            monitor_cpu_utilization();
        }
        else if (strcmp(argv[i], "get_load_average") == 0) {
            get_load_average();
        }
        else if (strcmp(argv[i], "get_cpu_temperature") == 0) {
            get_cpu_temperature();
        }
        else if (strcmp(argv[i], "get_gpu_temperature") == 0) {
            get_gpu_temperature();
        }
        else if (strcmp(argv[i], "get_vrm_temperature") == 0) {
            get_vrm_temperature();
        }
        else if (strcmp(argv[i], "get_chipset_temperature") == 0) {
            get_chipset_temperature();
        }
        else if (strcmp(argv[i], "get_motherboard_temperature") == 0) {
            get_motherboard_temperature();
        }
        else if (strcmp(argv[i], "get_psu_temperature") == 0) {
            get_psu_temperature();
        }
        else if (strcmp(argv[i], "get_case_temperature") == 0) {
            get_case_temperature();
        }
        else if (strcmp(argv[i], "find_storage_devices_with_temperature_reporting") == 0) {
            find_storage_devices_with_temperature_reporting();
        }
        else if (strcmp(argv[i], "detect_all_storage_devices") == 0) {
            detect_all_storage_devices();
        }
        else if (strcmp(argv[i], "print_smart_data") == 0) {
            print_smart_data();
        }
        else if (strcmp(argv[i], "display_running_processes") == 0) {
            display_running_processes();
        }
        else if (strcmp(argv[i], "display_hardware_info") == 0) {
            display_hardware_info();
        }
        else if (strcmp(argv[i], "print_kernel_details") == 0) {
            print_kernel_details();
        }
        else if (strcmp(argv[i], "print_distribution_info") == 0) {
            print_distribution_info();
        }
        else if (strcmp(argv[i], "print_library_versions") == 0) {
            print_library_versions();
        }
        else if (strcmp(argv[i], "print_security_info") == 0) {
            print_security_info();
        }
        else if (strcmp(argv[i], "detect_all_package_managers") == 0) {
            detect_all_package_managers();
        }
        else if (strcmp(argv[i], "list_manual_installs") == 0) {
            list_manual_installs();
        }
        else if (strcmp(argv[i], "check_startup_directories") == 0) {
            check_startup_directories();
        }
        else if (strcmp(argv[i], "check_systemd_user_services") == 0) {
            check_systemd_user_services();
        }
        else if (strcmp(argv[i], "show_system_uptime_and_cpu_sleep_time") == 0) {
            show_system_uptime_and_cpu_sleep_time();
        }
        else if (strcmp(argv[i], "get_total_jiffies") == 0) {
            get_total_jiffies();
        }
        else if (strcmp(argv[i], "check_firewall") == 0) {
            check_firewall();
        }
        else if (strcmp(argv[i], "show_logged_in_users") == 0) {
            show_logged_in_users();
        }
        else if (strcmp(argv[i], "view_system_logs") == 0) {
            view_system_logs();
        }
        else if (strcmp(argv[i], "read_journal_logs") == 0) {
            read_journal_logs();
        }
        else if (strcmp(argv[i], "get_total_cpu_time") == 0) {
            get_total_cpu_time();
        }
        else if (strcmp(argv[i], "print_uname_info") == 0) {
            print_uname_info();
        }
        else if (strcmp(argv[i], "print_detailed_os_info") == 0) {
            print_detailed_os_info();
        }
        else if (strcmp(argv[i], "print_system_limits") == 0) {
            print_system_limits();
        }
        else if (strcmp(argv[i], "scan_directory") == 0) {
            if (i + 1 < argc) {
                scan_directory(argv[i + 1]);
                i++; // Skip next argument since we used it as path
            } else {
                printf("Usage: %s scan_directory <path>\n", argv[0]);
            }
        }
        else {
            printf("Unknown command: %s\n", argv[i]);
            printf("Run without arguments to see available commands.\n");
            return 1;
        }
    }
    
    return 0;
}
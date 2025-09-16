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

// Global variables
StorageDevice *storage_devices = NULL;
int storage_device_count = 0;
CPUData cpu_data;
SystemHistory system_history;

/**
 * Initializes a history buffer with zeros and resets index/count
 */
void init_history_buffer(HistoryBuffer *buffer) {
    memset(buffer->values, 0, sizeof(buffer->values));
    buffer->index = 0;
    buffer->count = 0;
}

/**
 * Adds a new value to the circular history buffer
 * Maintains the most recent HISTORY_SIZE values
 */
void add_to_history(HistoryBuffer *buffer, float value) {
    buffer->values[buffer->index] = value;
    buffer->index = (buffer->index + 1) % HISTORY_SIZE;
    if (buffer->count < HISTORY_SIZE) {
        buffer->count++;
    }
}

/**
 * Calculates and returns the average of all values in the history buffer
 * Returns 0.0 if buffer is empty
 */
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

/**
 * Calculates CPU usage percentages for all cores based on delta between readings
 * Uses the formula: usage = (total_time - idle_time) / total_time * 100%
 */
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

/**
 * Calculates overall CPU usage by comparing idle time between two readings
 * Returns -1.0 if unable to read /proc/stat
 */
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
            return temp;
        }
    }
    
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
            return temp;
        }
    }

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
            return temp;
        }
    }

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
            return temp;
        }
    }

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

/**
 * Scans /sys/class/hwmon for storage devices (NVMe, SATA, SSD)
 * Populates the global storage_devices array with found devices
 */
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

/**
 * Reads temperature from a specific storage device path
 * Returns temperature in Celsius or -1.0 on error
 */
float get_storage_temperature(const char *path) {
    return read_temperature_file(path);
}

/**
 * Counts the number of running processes by scanning /proc directory
 * Returns -1 if /proc cannot be accessed
 */
int get_process_count() {
    DIR *dir = opendir("/proc");
    if (!dir) return -1;
    
    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (isdigit(entry->d_name[0])) {
            count++;
        }
    }
    closedir(dir);
    return count;
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
void detect_storage_devices(char ***devices, int *count) {
    const char *patterns[] = {
        "/dev/sd*", "/dev/nvme*n*", "/dev/mmcblk*", "/dev/vd*", "/dev/hd*", NULL
    };
    
    *count = 0;
    *devices = NULL;
    
    for (int i = 0; patterns[i] != NULL; i++) {
        glob_t glob_result;
        if (glob(patterns[i], GLOB_MARK, NULL, &glob_result) == 0) {
            for (size_t j = 0; j < glob_result.gl_pathc; j++) {
                char *path = glob_result.gl_pathv[j];
                if (strchr(path, 'p') == NULL && 
                    strspn(strrchr(path, '/') + 1, "0123456789") == 0) {
                    struct stat st;
                    if (stat(path, &st) == 0 && S_ISBLK(st.st_mode)) {
                        *devices = realloc(*devices, (*count + 1) * sizeof(char *));
                        (*devices)[*count] = strdup(path);
                        (*count)++;
                    }
                }
            }
            globfree(&glob_result);
        }
    }
}

/**
 * Run SmartCTL for the selected device and print its report
 */
void print_smart_data(const char *smartctl_path, const char *device) {
    char command[CMD_BUFFER_SIZE];
    FILE *pipe;

    snprintf(command, sizeof(command), "sudo %s -a %s 2>/dev/null", smartctl_path, device);
    printf("\n === S.M.A.R.T. Data for %s ===\n", device);
    printf("Executing: %s\n\n", command);

    pipe = popen(command, "r");
    if (pipe == NULL) {
        perror("popen failed");
        return;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        printf("%s", buffer);
    }
    pclose(pipe);
}

void print_device_list(char **devices, int count) {
    printf("Available storage devices:\n");
    printf("=============================\n");
    
    for (int i = 0; i < count; i++) {
        printf("%d. %s\n", i + 1, devices[i]);
    }
    printf("\n");
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
    printf("\n=== Distribution Information ===\n");
    
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
    
    for (int i = 0; package_managers[i] != NULL; i++) {
        FILE *fp = fopen(package_managers[i], "r");
        if (fp) {
            printf("Package Manager: %s\n", pm_names[i]);
            fclose(fp);
            break;
        }
        
        if (strstr(package_managers[i], "/etc/yum.repos.d/") || 
            strstr(package_managers[i], "/etc/emerge/")) {
            DIR *dir = opendir(package_managers[i]);
            if (dir) {
                printf("Package Manager: %s\n", pm_names[i]);
                closedir(dir);
                break;
            }
        }
    }
    
    printf("Init System: ");
    FILE *fp = fopen("/proc/1/comm", "r");
    if (fp) {
        char init_system[32];
        if (fgets(init_system, sizeof(init_system), fp)) {
            init_system[strcspn(init_system, "\n")] = 0;
            printf("%s", init_system);
        }
        fclose(fp);
    } else {
        FILE *cmd = popen("ps -p 1 -o comm= 2>/dev/null", "r");
        if (cmd) {
            char init_system[32];
            if (fgets(init_system, sizeof(init_system), cmd)) {
                init_system[strcspn(init_system, "\n")] = 0;
                printf("%s", init_system);
            }
            pclose(cmd);
        } else {
            char init_path[256];
            ssize_t len = readlink("/sbin/init", init_path, sizeof(init_path)-1);
            if (len != -1) {
                init_path[len] = '\0';
                char *basename = strrchr(init_path, '/');
                if (basename) {
                    printf("%s", basename + 1);
                } else {
                    printf("%s", init_path);
                }
            } else {
                printf("Unknown");
            }
        }
    }
    printf("\n");
    
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
    
    const char *distro_names[] = {
        "Debian",
        "RedHat",
        "CentOS",
        "Fedora", 
        "SUSE",
        "Arch",
        "Slackware",
        "Gentoo"
    };
    
    for (int i = 0; version_files[i] != NULL; i++) {
        fp = fopen(version_files[i], "r");
        if (fp) {
            char version[128];
            if (fgets(version, sizeof(version), fp)) {
                version[strcspn(version, "\n")] = 0;
                printf("%s Version: %s\n", distro_names[i], version);
            }
            fclose(fp);
            break;
        }
    }
    
    fp = fopen("/proc/1/comm", "r");
    if (fp) {
        char init_system[32];
        if (fgets(init_system, sizeof(init_system), fp)) {
            init_system[strcspn(init_system, "\n")] = 0;
            if (strcmp(init_system, "systemd") == 0) {
                FILE *cmd = popen("systemctl --version 2>/dev/null | head -1", "r");
                if (cmd) {
                    char systemd_version[64];
                    if (fgets(systemd_version, sizeof(systemd_version), cmd)) {
                        systemd_version[strcspn(systemd_version, "\n")] = 0;
                        printf("Systemd Version: %s\n", systemd_version);
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
}

void print_system_limits() {
    printf("\n=== System Limits ===\n");
    
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

void print_os_summary(void) {
    printf("=== OPERATING SYSTEM DETAILED SUMMARY ===\n");
    
    print_uname_info();
    print_detailed_os_info();
    print_kernel_details();
    print_distribution_info();
    print_library_versions();
    print_security_info();
    print_system_limits();
    
    printf("\n=== End of OS Summary ===\n");
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
            // Recurse into subdirectories
            scan_directory(fullpath);
        } else if (info.st_mode & S_IXUSR) {
            // Executable found
            printf("%s\n", fullpath);
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

    // Open the journalctl command
    fp = popen("sudo journalctl", "r");
    if (fp == NULL) {
        perror("Failed to run journalctl command");
        fprintf(stderr, "Error: Could not execute journalctl. Make sure you have sudo privileges.\n");
        return;
    }

    // Read the output line by line and print it
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        printf("Journal: %s", buffer);
    }

    // Check if there was an error during reading
    if (ferror(fp)) {
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
}

void *monitor_system(void *arg) {
    find_storage_devices();
    init_system_history();
    
    // Initialize CPU core monitoring
    cpu_data.total_cores = get_core_count();
    if (cpu_data.total_cores <= 0) {
        printf("Error: Could not determine number of CPU cores\n");
        return NULL;
    }

    // Initial read to populate stats
    read_cpu_stats(&cpu_data);
    sleep(2); // Wait for initial data

    const char *smartctl_path = find_smartctl_path();
    if (smartctl_path == NULL) {
        printf("Error: smartctl not found.\n");
        printf("Install with: sudo apt install smartmontools\n");
        return NULL;
    }

    char **devices = NULL;
    int device_count = 0;
    detect_storage_devices(&devices, &device_count);
    
    if (device_count == 0) {
        printf("No storage devices detected!\n");
        return NULL;
    }

     print_device_list(devices, device_count);
    
    for (int i = 0; i < device_count; i++) {
        print_smart_data(smartctl_path, devices[i]);
        free(devices[i]);
    }
    free(devices);

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
        int process_count = get_process_count();
        
        // Store in history
        add_to_history(&system_history.cpu_usage, cpu_usage);
        add_to_history(&system_history.load_1min, load.load_1min);
        add_to_history(&system_history.load_5min, load.load_5min);
        add_to_history(&system_history.load_15min, load.load_15min);
        add_to_history(&system_history.cpu_temp, cpu_temp);
        add_to_history(&system_history.gpu_temp, gpu_temp);
        add_to_history(&system_history.vrm_temp, vrm_temp);
        add_to_history(&system_history.chipset_temp, chipset_temp);
        add_to_history(&system_history.motherboard_temp, motherboard_temp);
        add_to_history(&system_history.psu_temp, psu_temp);
        add_to_history(&system_history.case_temp, case_temp);
        add_to_history(&system_history.total_processes, (float)process_count);
        
        // Read CPU core usage
        read_cpu_stats(&cpu_data);
        calculate_cpu_usage(&cpu_data);
        
        // Store core usage history
        for (int i = 0; i <= cpu_data.total_cores; i++) {
            add_to_history(&system_history.core_usage[i], cpu_data.cores[i].usage);
        }
        
        // Store storage temperatures
        system_history.storage_count = storage_device_count;
        for (int i = 0; i < storage_device_count && i < 16; i++) {
            float temp = get_storage_temperature(storage_devices[i].path);
                        add_to_history(&system_history.storage_temps[i], temp);
        }
        
        sleep(1); // Update every second
    }
    
    free(storage_devices);
    return NULL;
}

// Calculate total time computer was on in different states
long get_total_jiffies() {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return -1;
    char cpu[16];
    long user, nice, system, idle, iowait, irq, softirq, steal;
    fscanf(f, "%s %ld %ld %ld %ld %ld %ld %ld %ld",
           cpu, &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
    fclose(f);
    return user + nice + system + idle + iowait + irq + softirq + steal;
}

void read_process_stat(pid_t pid, unsigned long *utime, unsigned long *stime, long *rss) {
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

void read_process_io(pid_t pid, unsigned long *read_bytes, unsigned long *write_bytes) {
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
        // Process tree is not displayed in the main table to avoid cluttering
        // but the process count is already included in the main display
        sleep(5); // Update every 5 seconds
    }
    return NULL;
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

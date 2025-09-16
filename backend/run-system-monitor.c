#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

// All of the functions from the system-monitor.c file
void get_load_average();
void get_core_count();
void read_cpu_stats();
void calculate_cpu_usage();
void get_cpu_usage();
void get_cpu_temperature();
void get_gpu_temperature();
void get_vrm_temperature();
void get_chipset_temperature();
void get_motherboard_temperature();
void get_psu_temperature();
void get_case_temperature();
void find_storage_devices();
void get_storage_temperature();
void get_process_count();
void display_table_header();
void display_table_footer();
void display_history_row();
void calculate_total_rows();
void update_display();
void find_smartctl_path();
void detect_storage_devices();
void print_smart_data();
void print_device_list();
void print_uname_info();
void print_detailed_os_info();
void print_kernel_details();
void print_distribution_info();
void print_library_versions();
void print_security_info();
void print_system_limits();
void print_os_summary();
void check_startup_directories();
void check_systemd_user_services();
void show_system_uptime_and_cpu_sleep_time();
void detect_all_package_managers();
void scan_directory();
void list_manual_installs();
void read_journal_logs();
void monitor_system();
void get_total_jiffies();
void read_process_stat();
void read_process_io();
void list_processes();
void process_thread();
void show_system_uptime_and_cpu_sleep_time();

// Clear screen function
void clear_screen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

// Function to set terminal to raw mode
void set_raw_mode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Function to restore terminal to normal mode
void restore_terminal() {
    struct termios normal;
    tcgetattr(STDIN_FILENO, &normal);
    normal.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &normal);
}

// Function to handle function execution with backspace support
void run_function(void (*func)(), const char* func_name) {
    clear_screen();
    printf("\n--- Running %s ---\n", func_name);
    printf("Press Backspace to return to main menu\n\n");
    
    func();
    
    set_raw_mode();
    
    int c;
    while (1) {
        c = getchar();
        if (c == 127 || c == 8) { // Backspace or Delete key
            break;
        }
    }
    
    restore_terminal();
    clear_screen();
}

// Display a styled menu
void display_menu() {
    printf("\033[1;34m"); // Set text color to blue
    printf("╔═══════════════════════════════════════╗\n");
    printf("║         INTERACTIVE TERMINAL          ║\n");
    printf("║             APPLICATION               ║\n");
    printf("╠═══════════════════════════════════════╣\n");
    printf("║      Commands that can be run:        ║\n");
    printf("╠═══════════════════════════════════════╣\n");
    printf("║ Get Load Average\033[1;34m                      ║\n");
    printf("║ Get Core Count\033[1;34m                        ║\n");
    printf("║ Read CPU Stats\033[1;34m                        ║\n");
    printf("║ Calculate CPU Usage\033[1;34m                   ║\n");
    printf("║ Get CPU Usage\033[1;34m                         ║\n");
    printf("║ Get CPU Temperature\033[1;34m                   ║\n");
    printf("║ Get GPU Temperature\033[1;34m                   ║\n");
    printf("║ Get VRM Temperature\033[1;34m                   ║\n");
    printf("║ Get Chipset Temperature\033[1;34m               ║\n");
    printf("║ Get Motherboard Temperature\033[1;34m           ║\n");
    printf("║ Get PSU Temperature\033[1;34m                   ║\n");
    printf("║ Get Case Temperature\033[1;34m                  ║\n");
    printf("║ Find Storage Devices\033[1;34m                  ║\n");
    printf("║ Get Storage Temperature\033[1;34m               ║\n");
    printf("║ Get Process Count\033[1;34m                     ║\n");
    printf("║ Detect Storage Devices\033[1;34m                ║\n");
    printf("║ Print Smart Data\033[1;34m                      ║\n");
    printf("║ Print Device List\033[1;34m                     ║\n");
    printf("║ Print Uname Info\033[1;34m                      ║\n");
    printf("║ Print Detailed OS Info\033[1;34m                ║\n");
    printf("║ Print Kernel Details\033[1;34m                  ║\n");
    printf("║ Print Distribution Info\033[1;34m               ║\n");
    printf("║ Print Library Versions\033[1;34m                ║\n");
    printf("║ Print Security Info\033[1;34m                   ║\n");
    printf("║ Print System Limits\033[1;34m                   ║\n");
    printf("║ Print OS Summary\033[1;34m                      ║\n");
    printf("║ Print Startup Directories\033[1;34m             ║\n");
    printf("║ Check Systemd User Services\033[1;34m           ║\n");
    printf("║ Show System Uptime And CPU Sleep Time\033[1;34m ║\n");
    printf("║ Detect All Package Managers\033[1;34m           ║\n");
    printf("║ Scan Directory\033[1;34m                        ║\n");
    printf("║ List Manual Installs\033[1;34m                  ║\n");
    printf("║ Read Journal Logs\033[1;34m                     ║\n");
    printf("║ Monitor System\033[1;34m                        ║\n");
    printf("║ Exit\033[1;34m                                  ║\n");
    printf("╚═══════════════════════════════════════╝\n");
    printf("\033[0m"); // Reset text color
    printf("\nSelect an option (Type the function name or exit): ");
}

int main() {
    char input[100];
    
    while (1) {
        clear_screen();
        display_menu();
        
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        
        // Remove newline character
        input[strcspn(input, "\n")] = 0;
        
        if (strcmp(input, "Get Load Average") == 0) {
            run_function(get_load_average, "Get Load Average");
        } else if (strcmp(input, "Get Core Count") == 0) {
            run_function(get_core_count, "Get Core Count");
        } else if (strcmp(input, "Read CPU Stats") == 0) {
            run_function(read_cpu_stats, "Read CPU Stats");
        } else if (strcmp(input, "Calculate CPU Usage") == 0) {
            run_function(calculate_cpu_usage, "Calculate CPU Usage");
        } else if (strcmp(input, "Get CPU Usage") == 0) {
            run_function(get_cpu_usage, "Get CPU Usage");
        } else if (strcmp(input, "Get CPU Temperature") == 0) {
            run_function(get_cpu_temperature, "Get CPU Temperature");
        } else if (strcmp(input, "Get GPU Temperature") == 0) {
            run_function(get_gpu_temperature, "Get GPU Temperature");
        } else if (strcmp(input, "Get VRM Temperature") == 0) {
            run_function(get_vrm_temperature, "Get VRM Temperature");
        } else if (strcmp(input, "Get Chipset Temperature") == 0) {
            run_function(get_chipset_temperature, "Get Chipset Temperature");
        } else if (strcmp(input, "Get Motherboard Temperature") == 0) {
            run_function(get_motherboard_temperature, "Get Motherboard Temperature");
        } else if (strcmp(input, "Get PSU Temperature") == 0) {
            run_function(get_psu_temperature, "Get PSU Temperature");
        } else if (strcmp(input, "Get Case Temperature") == 0) {
            run_function(get_case_temperature, "Get Case Temperature");
        } else if (strcmp(input, "Find Storage Devices") == 0) {
            run_function(find_storage_devices, "Find Storage Devices");
        } else if (strcmp(input, "Get Storage Temperature") == 0) {
            run_function(get_storage_temperature, "Get Storage Temperature");
        } else if (strcmp(input, "Get Process Count") == 0) {
            run_function(get_process_count, "Get Process Count");
        } else if (strcmp(input, "Detect Storage Devices") == 0) {
            run_function(detect_storage_devices, "Detect Storage Devices");
        } else if (strcmp(input, "Print Smart Data") == 0) {
            run_function(print_smart_data, "Print Smart Data");
        } else if (strcmp(input, "Print Device List") == 0) {
            run_function(print_device_list, "Print Device List");
        } else if (strcmp(input, "Print Uname Info") == 0) {
            run_function(print_uname_info, "Print Uname Info");
        } else if (strcmp(input, "Print Detailed OS Info") == 0) {
            run_function(print_detailed_os_info, "Print Detailed OS Info");
        } else if (strcmp(input, "Print Kernel Details") == 0) {
            run_function(print_kernel_details, "Print Kernel Details");
        } else if (strcmp(input, "Print Distribution Info") == 0) {
            run_function(print_distribution_info, "Print Distribution Info");
        } else if (strcmp(input, "Print Library Versions") == 0) {
            run_function(print_library_versions, "Print Library Versions");
        } else if (strcmp(input, "Print Security Info") == 0) {
            run_function(print_security_info, "Print Security Info");
        } else if (strcmp(input, "Print System Limits") == 0) {
            run_function(print_system_limits, "Print System Limits");
        } else if (strcmp(input, "Print OS Summary") == 0) {
            run_function(print_os_summary, "Print OS Summary");
        } else if (strcmp(input, "Print Startup Directories") == 0) {
            run_function(check_startup_directories, "Print Startup Directories");
        } else if (strcmp(input, "Check Systemd User Services") == 0) {
            run_function(check_systemd_user_services, "Check Systemd User Services");
        } else if (strcmp(input, "Show System Uptime And CPU Sleep Time") == 0) {
            run_function(show_system_uptime_and_cpu_sleep_time, "Show System Uptime And CPU Sleep Time");
        } else if (strcmp(input, "Detect All Package Managers") == 0) {
            run_function(detect_all_package_managers, "Detect All Package Managers");
        } else if (strcmp(input, "Scan Directory") == 0) {
            run_function(scan_directory, "Scan Directory");
        } else if (strcmp(input, "List Manual Installs") == 0) {
            run_function(list_manual_installs, "List Manual Installs");
        } else if (strcmp(input, "Read Journal Logs") == 0) {
            run_function(read_journal_logs, "Read Journal Logs");
        } else if (strcmp(input, "Monitor System") == 0) {
            run_function(monitor_system, "Monitor System");         
        } else if (strcmp(input, "exit") == 0) {
            clear_screen();
            printf("\033[1;32m"); // Green text
            printf("Thank you for using the application!\n");
            printf("Goodbye!\n");
            printf("\033[0m"); // Reset text color
            break;
        } else {
            printf("\033[1;31mInvalid option. Please try again.\033[0m\n");
            usleep(1500000); // Pause for 1.5 seconds
        }
    }
    
    return 0;
}
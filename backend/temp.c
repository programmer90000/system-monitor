#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUFFER 1024

void display_hardware_info() {
    // Helper function to run commands
    void run_command(const char *command, const char *description) {
        printf("\n\033[1;34m%s\033[0m\n", description);
        printf("\033[1;32m");
        for (int i = 0; i < strlen(description); i++) printf("=");
        printf("\033[0m\n");
        
        FILE *fp = popen(command, "r");
        if (fp == NULL) {
            printf("Failed to run command: %s\n", command);
            return;
        }
        
        char buffer[MAX_BUFFER];
        while (fgets(buffer, sizeof(buffer), fp) != NULL) {
            printf("%s", buffer);
        }
        
        pclose(fp);
        printf("\n");
    }

    printf("===============================================================================================================================================");
    run_command("sudo lshw", "SYSTEM HARDWARE OVERVIEW (COMPLETE)"); // More detailed than -short
    printf("===============================================================================================================================================");
    run_command("sudo lspci -vvv", "PCI DEVICES DETAILED INFORMATION"); // Triple verbose
    printf("===============================================================================================================================================");
    run_command("sudo lsusb -v", "USB DEVICES DETAILED INFORMATION"); // Verbose mode
    printf("===============================================================================================================================================");
    run_command("sudo dmidecode", "COMPLETE DMI INFORMATION"); // All DMI data without filtering
    printf("===============================================================================================================================================");
    run_command("sudo dmidecode -t baseboard", "MOTHERBOARD INFORMATION (DMI)");
    printf("===============================================================================================================================================");
    run_command("sudo dmidecode -t memory", "MEMORY INFORMATION (DMI)");
    printf("===============================================================================================================================================");
    run_command("lscpu -e", "CPU DETAILED INFORMATION (EXTENDED)"); // Extended format
    printf("===============================================================================================================================================");
    run_command("lsblk -a -o NAME,SIZE,TYPE,MOUNTPOINT,FSTYPE,MODEL,SERIAL,STATE", "STORAGE DEVICES COMPLETE INFORMATION");
    printf("===============================================================================================================================================");
    run_command("df -a -T", "COMPLETE DISK USAGE INFORMATION"); // All filesystems with types
    printf("===============================================================================================================================================");
    run_command("sudo hwinfo --short", "HARDWARE DETECTION (COMPLETE)"); // Additional hardware info
    printf("===============================================================================================================================================");
    run_command("lsmod", "LOADED KERNEL MODULES");
    printf("===============================================================================================================================================");
    run_command("sudo lscpu -x", "CPU INFORMATION (EXTENDED)");
    printf("===============================================================================================================================================");
    run_command("free -h", "MEMORY USAGE INFORMATION");
    printf("===============================================================================================================================================");
    run_command("cat /proc/cpuinfo", "CPU INFO FROM /PROC");
    printf("===============================================================================================================================================");
    run_command("cat /proc/meminfo", "MEMORY INFO FROM /PROC");
    printf("===============================================================================================================================================");
    run_command("sudo smartctl --scan", "STORAGE HEALTH INFORMATION");
    printf("===============================================================================================================================================");
    run_command("ip link show", "NETWORK INTERFACES");
    printf("===============================================================================================================================================");
    run_command("sudo dmidecode -t bios", "BIOS INFORMATION");
    printf("===============================================================================================================================================");
    run_command("sudo dmidecode -t processor", "PROCESSOR DETAILS");
    printf("===============================================================================================================================================");
    run_command("sudo dmidecode -t chassis", "CHASSIS INFORMATION");
    printf("===============================================================================================================================================");
    // Additional comprehensive commands (some may need installation)
    run_command("inxi -Fxxxz", "COMPREHENSIVE SYSTEM INFORMATION"); // Requires inxi package
    printf("===============================================================================================================================================");
    run_command("hwinfo --all", "ALL HARDWARE INFORMATION"); // Requires hwinfo package
    printf("===============================================================================================================================================");
}

int main() {
    display_hardware_info();
    return 0;
}
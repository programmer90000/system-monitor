#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUFFER 1024

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

int main() {
    printf("\033[1;36m");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                   HARDWARE INFORMATION                       ║\n");
    printf("║                   Comprehensive System Scan                  ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\033[0m");
    
    run_command("sudo lshw -short", "SYSTEM HARDWARE OVERVIEW");
    run_command("sudo lspci -tv", "PCI DEVICES TREE VIEW");
    run_command("sudo lsusb -tv", "USB DEVICES TREE VIEW");
    run_command("sudo dmidecode -t system", "SYSTEM INFORMATION (DMI)");
    run_command("sudo dmidecode -t baseboard", "MOTHERBOARD INFORMATION (DMI)");
    run_command("sudo dmidecode -t memory", "MEMORY INFORMATION (DMI)");
    run_command("lscpu", "CPU DETAILED INFORMATION");
    run_command("lsblk", "STORAGE DEVICES AND PARTITIONS");
    run_command("df -h", "DISK USAGE INFORMATION");
    
    printf("\033[1;35m");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                  SCAN COMPLETE                               ║\n");
    printf("║                  All hardware information has been displayed ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\033[0m");
    
    return 0;
}
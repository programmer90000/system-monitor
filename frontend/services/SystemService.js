import React from "react";
import { runCommand, runSudoCommand } from "../src/lib/run-commands.js";

const getHardwareData = () => {
    Promise.allSettled([
        runCommand("get_core_count", []).then((output) => {
            const coreText = output.replace(/^"|\s*"$/g, "").trim();
            const coreMatch = coreText.match(/Total cores:\s*(\d+)/);
            const coreCount = coreMatch ? coreMatch[1] : "N/A";
            console.log("=== Core Count ===");
            console.log(coreCount);
            console.log("\n");
            return { "type": "coreCount", "value": output };
        }),

        runCommand("calculate_cpu_usage", []).then((output) => {
            const cpuText = output.replace(/^"|\s*"$/g, "").trim();
            // Get the SECOND CPU usage value (the actual measurement)
            const cpuMatches = cpuText.match(/CPU Usage:\s*([\d.]+)%/g);
            let cpuUsage = "N/A";
            if (cpuMatches && cpuMatches.length > 1) {
                // Take the second occurrence which is the actual measurement
                const secondMatch = cpuMatches[1].match(/CPU Usage:\s*([\d.]+)%/);
                cpuUsage = secondMatch ? secondMatch[1] : "N/A";
            } else if (cpuMatches && cpuMatches.length === 1) {
                // If only one measurement, use it
                const singleMatch = cpuMatches[0].match(/CPU Usage:\s*([\d.]+)%/);
                cpuUsage = singleMatch ? singleMatch[1] : "N/A";
            }
            console.log("=== CPU Usage ===");
            console.log(`${cpuUsage}%`);
            console.log("\n");
            return { "type": "cpuUsage", "value": output };
        }),

        runCommand("read_cpu_stats", []).then((output) => {
            const statsText = output.replace(/^"|\s*"$/g, "").trim();
            const lines = statsText.split("\n");
            lines.forEach((line) => {
                if (line.includes(":")) {
                    const [cpuName, stats] = line.split(":");
                    const statMatches = stats.match(/user=(\d+)\s+nice=(\d+)\s+system=(\d+)\s+idle=(\d+)\s+iowait=(\d+)\s+irq=(\d+)\s+softirq=(\d+)\s+steal=(\d+)/);
                    if (statMatches) {
                        console.log("=== CPU Stats ===");
                        console.log(`Name: ${cpuName.trim()}:`);
                        console.log(`User: ${statMatches[1]}`);
                        console.log(`Nice: ${statMatches[2]}`);
                        console.log(`System: ${statMatches[3]}`);
                        console.log(`Idle: ${statMatches[4]}`);
                        console.log(`IOWait: ${statMatches[5]}`);
                        console.log(`IRQ: ${statMatches[6]}`);
                        console.log(`SoftIRQ: ${statMatches[7]}`);
                        console.log(`Steal: ${statMatches[8]}`);
                        console.log("\n");
                    }
                }
            });
            return { "type": "cpuStats", "value": output };
        }),

        runCommand("monitor_cpu_utilization", []).then((output) => {
            const utilText = output.replace(/^"|\s*"$/g, "").trim();
            const timestampMatch = utilText.match(/Timestamp:\s*(\S+)/);
            const userMatch = utilText.match(/User:\s*([\d.]+)%/);
            const systemMatch = utilText.match(/System:\s*([\d.]+)%/);
            const ioWaitMatch = utilText.match(/IOWait:\s*([\d.]+)%/);
            const totalMatch = utilText.match(/Total:\s*([\d.]+)%/);
            console.log("=== CPU Utilization ===");
            console.log("  Timestamp:", timestampMatch ? timestampMatch[1] : "N/A");
            console.log("  User:", userMatch ? `${userMatch[1]}%` : "N/A");
            console.log("  System:", systemMatch ? `${systemMatch[1]}%` : "N/A");
            console.log("  IOWait:", ioWaitMatch ? `${ioWaitMatch[1]}%` : "N/A");
            console.log("  Total:", totalMatch ? `${totalMatch[1]}%` : "N/A");
            console.log("\n");
            return { "type": "cpuUtilization", "value": output };
        }),

        runCommand("get_load_average", []).then((output) => {
            const loadText = output.replace(/^"|\s*"$/g, "").trim();
            const load1Match = loadText.match(/1 minute\s*:\s*([\d.]+)/);
            const load5Match = loadText.match(/5 minutes\s*:\s*([\d.]+)/);
            const load15Match = loadText.match(/15 minutes\s*:\s*([\d.]+)/);
            const load1min = load1Match ? load1Match[1] : "N/A";
            const load5min = load5Match ? load5Match[1] : "N/A";
            const load15min = load15Match ? load15Match[1] : "N/A";
            console.log("=== Load Average ===");
            console.log("1 minute:", load1min);
            console.log("5 minutes:", load5min);
            console.log("15 minutes:", load15min);
            console.log("\n");
            return { "type": "loadAverage", "value": output };
        }),

        runCommand("display_hardware_info", []).then((output) => {
            const hardwareText = output.replace(/^"|\s*"$/g, "").trim();
            // Extract key system information
            const hostnameMatch = hardwareText.match(/Static hostname:\s*(\S+)/);
            const osMatch = hardwareText.match(/Operating System:\s*([^\n]+)/);
            const kernelMatch = hardwareText.match(/Kernel:\s*([^\n]+)/);
            const archMatch = hardwareText.match(/Architecture:\s*([^\n]+)/);
            const vendorMatch = hardwareText.match(/Hardware Vendor:\s*([^\n]+)/);
            const modelMatch = hardwareText.match(/Hardware Model:\s*([^\n]+)/);
            const biosMatch = hardwareText.match(/Firmware Version:\s*([^\n]+)/);
            // Extract CPU information
            const cpuModelMatch = hardwareText.match(/model name\s*:\s*([^\n]+)/);
            const cpuCoresMatch = hardwareText.match(/cpu cores\s*:\s*(\d+)/);
            const cpuMhzMatch = hardwareText.match(/cpu MHz\s*:\s*([\d.]+)/);
            console.log("=== Hardware Info ===");
            console.log("Hostname:", hostnameMatch ? hostnameMatch[1] : "N/A");
            console.log("OS:", osMatch ? osMatch[1].trim() : "N/A");
            console.log("Kernel:", kernelMatch ? kernelMatch[1].trim() : "N/A");
            console.log("Architecture:", archMatch ? archMatch[1].trim() : "N/A");
            console.log("Vendor:", vendorMatch ? vendorMatch[1].trim() : "N/A");
            console.log("Model:", modelMatch ? modelMatch[1].trim() : "N/A");
            console.log("BIOS/Firmware:", biosMatch ? biosMatch[1].trim() : "N/A");
                    
            console.log("CPU:");
            console.log("Model:", cpuModelMatch ? cpuModelMatch[1].trim() : "N/A");
            console.log("Cores:", cpuCoresMatch ? cpuCoresMatch[1] : "N/A");
            console.log("Frequency:", cpuMhzMatch ? `${cpuMhzMatch[1]} MHz` : "N/A");
            console.log("\n");
            return { "type": "hardwareInfo", "value": output };
        }),

        runCommand("show_system_uptime_and_cpu_sleep_time", []).then((output) => {
            const uptimeText = output.replace(/^"|\s*"$/g, "").trim();
            const uptimeMatch = uptimeText.match(/System Uptime:\s*([\d.]+)\s*seconds/);
            const sleepMatch = uptimeText.match(/CPU Sleep Time:\s*([\d.]+)\s*seconds/);
            const systemUptime = uptimeMatch ? uptimeMatch[1] : "N/A";
            const cpuSleepTime = sleepMatch ? sleepMatch[1] : "N/A";
                    
            // Convert to hours for better readability
            const uptimeHours = systemUptime !== "N/A" ? (systemUptime / 3600).toFixed(2) : "N/A";
            const sleepHours = cpuSleepTime !== "N/A" ? (cpuSleepTime / 3600).toFixed(2) : "N/A";
            console.log("=== System Uptime And CPU Sleep Time ===:");
            console.log("System Uptime:", `${systemUptime} seconds (${uptimeHours} hours)`);
            console.log("CPU Sleep Time:", `${cpuSleepTime} seconds (${sleepHours} hours)`);
            console.log("\n");
            return { "type": "uptime", "value": output };
        }),

        runCommand("get_total_jiffies", []).then((output) => {
            const jiffiesText = output.replace(/^"|\s*"$/g, "").trim();
            const jiffiesMatch = jiffiesText.match(/Total Jiffies:\s*(\d+)/);
            const totalJiffies = jiffiesMatch ? jiffiesMatch[1] : "N/A";
            // Parse detailed CPU statistics
            const userMatch = jiffiesText.match(/User:\s*(\d+)/);
            const niceMatch = jiffiesText.match(/Nice:\s*(\d+)/);
            const systemMatch = jiffiesText.match(/System:\s*(\d+)/);
            const idleMatch = jiffiesText.match(/Idle:\s*(\d+)/);
            const ioWaitMatch = jiffiesText.match(/IOWait:\s*(\d+)/);
            const irqMatch = jiffiesText.match(/IRQ:\s*(\d+)/);
            const softIrqMatch = jiffiesText.match(/SoftIRQ:\s*(\d+)/);
            const stealMatch = jiffiesText.match(/Steal:\s*(\d+)/);
            console.log("=== Total Jiffies ===");
            console.log("Parsed Total Jiffies:", totalJiffies);
            console.log("Detailed CPU Time Breakdown:");
            console.log("  User:", userMatch ? userMatch[1] : "N/A");
            console.log("  Nice:", niceMatch ? niceMatch[1] : "N/A");
            console.log("  System:", systemMatch ? systemMatch[1] : "N/A");
            console.log("  Idle:", idleMatch ? idleMatch[1] : "N/A");
            console.log("  IOWait:", ioWaitMatch ? ioWaitMatch[1] : "N/A");
            console.log("  IRQ:", irqMatch ? irqMatch[1] : "N/A");
            console.log("  SoftIRQ:", softIrqMatch ? softIrqMatch[1] : "N/A");
            console.log("  Steal:", stealMatch ? stealMatch[1] : "N/A");
            console.log("\n");
            return { "type": "totalJiffies", "value": output };
        }),

        runCommand("get_total_cpu_time", []).then((output) => {
            const cpuTimeText = output.replace(/^"|\s*"$/g, "").trim();
            const totalTimeMatch = cpuTimeText.match(/Total CPU time:\s*(\d+)\s*jiffies/);
            const totalCpuTime = totalTimeMatch ? totalTimeMatch[1] : "N/A";
            // Parse detailed component breakdown
            const userMatch = cpuTimeText.match(/User mode:\s*(\d+)/);
            const niceMatch = cpuTimeText.match(/Nice mode:\s*(\d+)/);
            const systemMatch = cpuTimeText.match(/System mode:\s*(\d+)/);
            const idleMatch = cpuTimeText.match(/Idle time:\s*(\d+)/);
            const ioWaitMatch = cpuTimeText.match(/I\/O wait:\s*(\d+)/);
            const irqMatch = cpuTimeText.match(/IRQ time:\s*(\d+)/);
            const softIrqMatch = cpuTimeText.match(/Soft IRQ:\s*(\d+)/);
            const stealMatch = cpuTimeText.match(/Steal time:\s*(\d+)/);
            console.log("=== Total CPU Time ===");
            console.log("Parsed Total CPU Time:", `${totalCpuTime} jiffies`);
            console.log("User mode:", userMatch ? userMatch[1] : "N/A");
            console.log("Nice mode:", niceMatch ? niceMatch[1] : "N/A");
            console.log("System mode:", systemMatch ? systemMatch[1] : "N/A");
            console.log("Idle time:", idleMatch ? idleMatch[1] : "N/A");
            console.log("I/O wait:", ioWaitMatch ? ioWaitMatch[1] : "N/A");
            console.log("IRQ time:", irqMatch ? irqMatch[1] : "N/A");
            console.log("Soft IRQ:", softIrqMatch ? softIrqMatch[1] : "N/A");
            console.log("Steal time:", stealMatch ? stealMatch[1] : "N/A");
            return { "type": "totalCpuTime", "value": output };
        }),

    ]).then((results) => {
        results.forEach((result) => {
            if (result.status === "fulfilled") {
            }
            if (result.status === "rejected") {
                console.log(`${result.reason?.type || "Unknown"}: Failed to collect data`);
            }
        });
    });
};

const getLogs = (setSystemLogs, setJournalLogs) => {
    Promise.allSettled([
        runCommand("view_system_logs", []).then((output) => {
            setSystemLogs(output);
            return { "type": "systemLogs", "value": output };
        }),
        runSudoCommand("read_journal_logs", []).then((output) => {
            setJournalLogs(output);
            return { "type": "journalLogs", "value": output };
        }),

    ]).then((results) => {

        results.forEach((result, index) => {
            if (result.status === "fulfilled") {
                console.log(result);
            }
            if (result.status === "rejected") {
                console.error(`Command ${index} failed:`, result.reason);
            }
        });
    });
};


const getOsInformation = (setOsInformation, setParsedData) => {
    Promise.allSettled([
        runCommand("print_detailed_os_info", []).then((output) => {
            setOsInformation((prev) => { return { ...prev, "osInfo": output }; });
            return { "type": "osInfo", "value": output };
        }),

        runCommand("print_distribution_info", []).then((output) => {
            setOsInformation((prev) => { return { ...prev, "distributionInfo": output }; });
            return { "type": "distributionInfo", "value": output };
        }),

        runCommand("print_kernel_details", []).then((output) => {
            setOsInformation((prev) => { return { ...prev, "kernelDetails": output }; });
            return { "type": "kernelDetails", "value": output };
        }),

        runCommand("print_library_versions", []).then((output) => {
            setOsInformation((prev) => { return { ...prev, "libraryVersions": output }; });
            return { "type": "libraryVersions", "value": output };
        }),

        runCommand("print_security_info", []).then((output) => {
            setOsInformation((prev) => { return { ...prev, "securityInfo": output }; });
            return { "type": "securityInfo", "value": output };
        }),

        runCommand("print_system_limits", []).then((output) => {
            setOsInformation((prev) => { return { ...prev, "systemLimits": output }; });
            return { "type": "systemLimits", "value": output };
        }),

        runCommand("print_uname_info", []).then((output) => {
            setOsInformation((prev) => { return { ...prev, "unameInfo": output }; });
            return { "type": "unameInfo", "value": output };
        }),

    ]).then((results) => {
        const allData = {};
        results.forEach((result, index) => {
            if (result.status === "fulfilled") {
                allData[result.value.type] = result.value.value;
            }
            if (result.status === "rejected") {
                console.error(`Command ${index} failed:`, result.reason);
            }
        });

        const parsed = {};
        if (allData.osInfo) {
            const osInfoText = allData.osInfo.replace(/^"|\s*"$/g, "").trim();
            parsed.osInfo = {
                "prettyName": osInfoText.match(/PRETTY_NAME="([^"]+)"/)?.[1],
                "name": osInfoText.match(/NAME="([^"]+)"/)?.[1],
                "versionId": osInfoText.match(/VERSION_ID="([^"]+)"/)?.[1],
                "version": osInfoText.match(/VERSION="([^"]+)"/)?.[1],
                "versionCodename": osInfoText.match(/VERSION_CODENAME=([^\s]+)/)?.[1],
                "id": osInfoText.match(/ID=([^\s]+)/)?.[1],
                "homeUrl": osInfoText.match(/HOME_URL="([^"]+)"/)?.[1],
                "supportUrl": osInfoText.match(/SUPPORT_URL="([^"]+)"/)?.[1],
                "bugReportUrl": osInfoText.match(/BUG_REPORT_URL="([^"]+)"/)?.[1],
            };
        }

        if (allData.distributionInfo) {
            const distroLines = allData.distributionInfo.replace(/^"|\s*"$/g, "").trim().split("\n");
            parsed.distributionInfo = {
                "packageManager": distroLines[0] || "",
                "initSystem": distroLines[1] || "",
                "debianVersion": distroLines[2] || "",
                "systemdVersion": distroLines[3] || "",
            };
        }

        if (allData.kernelDetails) {
            const kernelText = allData.kernelDetails.replace(/^"|\s*"$/g, "").trim();
            parsed.kernelDetails = {
                "fullVersion": kernelText.match(/Full Kernel Version:\s*(.+?)(?=\n|$)/)?.[1]?.trim(),
                "commandLine": kernelText.match(/Kernel Command Line:\s*(.+?)(?=\n|$)/)?.[1]?.trim(),
                "architecture": kernelText.match(/Kernel Architecture:\s*(.+?)(?=\n|$)/)?.[1]?.trim(),
            };
        }

        if (allData.libraryVersions) {
            const libText = allData.libraryVersions.replace(/^"|\s*"$/g, "").trim();
            parsed.libraryVersions = {
                "glibcVersion": libText.match(/GLIBC Version:\s*([^\n]+)/)?.[1],
                "glibcRelease": libText.match(/GLIBC Release:\s*([^\n]+)/)?.[1],
                "usingGlibc": libText.match(/Using GLIBC:\s*([^\n]+)/)?.[1],
                "gccVersion": libText.match(/GCC Version:\s*([^\n]+)/)?.[1],
                "cStandard": libText.match(/C Standard:\s*([^\n]+)/)?.[1],
            };
        }

        if (allData.securityInfo) {
            const securityText = allData.securityInfo.replace(/^"|\s*"$/g, "").trim();
            parsed.securityInfo = {
                "securityUpdatesConfigured": securityText.match(/Security updates configured:\s*([^\n]+)/)?.[1] === "Yes",
                "securityModules": securityText.match(/Security Modules:\s*([^\n]+)/)?.[1]?.split(",") || [],
            };
        }

        if (allData.systemLimits) {
            const limitsText = allData.systemLimits.replace(/^"|\s*"$/g, "").trim();
            parsed.systemLimits = {
                "maxPid": parseInt(limitsText.match(/Maximum PID:\s*([^\n]+)/)?.[1]) || 0,
                "maxThreads": parseInt(limitsText.match(/Maximum threads:\s*([^\n]+)/)?.[1]) || 0,
                "maxPtys": parseInt(limitsText.match(/Maximum PTYs:\s*([^\n]+)/)?.[1]) || 0,
            };
        }

        if (allData.unameInfo) {
            const unameText = allData.unameInfo.replace(/^"|\s*"$/g, "").trim();
            parsed.unameInfo = {
                "os": unameText.match(/OS:\s*([^\n]+)/)?.[1],
                "hostname": unameText.match(/Hostname:\s*([^\n]+)/)?.[1],
                "kernelRelease": unameText.match(/Kernel Release:\s*([^\n]+)/)?.[1],
                "kernelVersion": unameText.match(/Kernel Version:\s*([^\n]+)/)?.[1],
                "architecture": unameText.match(/Architecture:\s*([^\n]+)/)?.[1],
            };
        }

        setParsedData(parsed);
        console.log("Parsed System Info:", parsed);
    });
};

const getPackageManagers = (setPackageManagers, setParsedData) => {
    runCommand("detect_all_package_managers", [])
        .then((output) => {
            setPackageManagers(output);

            const parse = (logs) => {
                const result = {};
                if (!logs) { return result; }
                const lines = logs.split("\n");
                let currentManager = null;
                for (const line of lines) {
                    const managerMatch = line.match(/(apt|yum|dnf|pacman|zypper|brew|choco|winget) detected:/);
                    if (managerMatch) {
                        currentManager = managerMatch[1];
                        const versionMatch = line.match(/: (.+)$/);
                        result[currentManager] = {
                            "version": versionMatch ? versionMatch[1].trim() : null,
                            "available": true,
                            "packages": {},
                        };
                        continue;
                    }
                    const notFoundMatch = line.match(/(apt|yum|dnf|pacman|zypper|brew|choco|winget) detected: sh: .*not found/);
                    if (notFoundMatch) {
                        const manager = notFoundMatch[1];
                        result[manager] = {
                            "available": false,
                            "error": "Command not found",
                            "packages": {},
                        };
                        continue;
                    }
                    if (currentManager && result[currentManager] && result[currentManager].available && line.trim() && !line.includes("WARNING:") && !line.includes("Listing...") && !line.includes("Installed packages for") && !line.match(/sh: .*not found/)) {
                        const packageMatch = line.match(/^\s*([^\/\s]+)\/([^,\s]+)(?:,([^ ]+))?\s+([^ ]+)\s+([^ ]+)\s+(\[.+\])?/);
                        if (packageMatch) {
                            const [, packageName, version, status, repo, arch, flags] = packageMatch;
                            const cleanPackageName = packageName.trim();
                            result[currentManager].packages[cleanPackageName] = {
                                "version": version.trim(),
                                "status": status ? status.trim() : "",
                                "repository": repo.trim(),
                                "architecture": arch.trim(),
                                "flags": flags ? flags.replace(/[\[\]]/g, "").split(",").map((f) => { return f.trim(); }) : [],
                            };
                        } else if (line.trim()) {
                            const rawLine = line.trim();
                            const fallbackName = rawLine.split("/")[0]?.trim() || rawLine;
                            result[currentManager].packages[fallbackName] = {
                                "raw": rawLine,
                                "version": "unknown",
                                "status": "unknown",
                                "repository": "unknown",
                                "architecture": "unknown",
                                "flags": [],
                            };
                        }
                    }
                }
                return result;
            };

            const parsed = parse(output);
            setParsedData(parsed);
            console.log("Parsed Package Managers:", parsed);
        });
};

const getStorageInfo = (setStorageDevices, setStorageDevicesWithTemperatureReporting, setSmartData, setParsedData) => {
    Promise.allSettled([
        runCommand("detect_all_storage_devices", []).then((output) => {
            const parseDevices = (out) => {
                const devices = [];
                out.split("\n").forEach((line) => {
                    const trimmed = line.trim();
                    if (trimmed.startsWith("Detected storage device:")) {
                        const devicePath = trimmed.replace("Detected storage device:", "").trim();
                        if (devicePath) {
                            devices.push({
                                "path": devicePath,
                                "type": devicePath.includes("nvme") ? "NVMe" : devicePath.includes("sd") ? "SATA" : devicePath.includes("hd") ? "IDE" : "Unknown",
                            });
                        }
                    }
                });
                return devices;
            };
            const parsed = parseDevices(output);
            setStorageDevices(parsed);
            return { "type": "detectAllStorageDevices", "value": output, parsed };
        }),
        runCommand("find_storage_devices_with_temperature_reporting", []).then((output) => {
            const parseTempDevices = (out) => {
                const devices = [];
                out.split("\n").forEach((line) => {
                    const trimmed = line.trim();
                    if (trimmed.startsWith("Storage Device Name:")) {
                        const match = trimmed.match(/Storage Device Name: (\w+) Temperature: ([\d.]+)°C/);
                        if (match) {
                            devices.push({ "name": match[1], "temperature": parseFloat(match[2]), "temperatureUnit": "°C" });
                        }
                    }
                });
                return devices;
            };
            const parsed = parseTempDevices(output);
            setStorageDevicesWithTemperatureReporting(parsed);
            return { "type": "findStorageDevicesWithTemperatureReporting", "value": output, parsed };
        }),
        runSudoCommand("print_smart_data", []).then((output) => {
            const parseSmart = (out) => {
                const result = {};
                const lines = out.split("\n");
                let currentDevice = "";
                let deviceData = {};
                let inInformationSection = false;
                let inSmartDataSection = false;
                let inPowerStates = false;
                let inLbaSizes = false;
                let inErrorLog = false;
                lines.forEach((line) => {
                    const trimmed = line.trim();
                    if (trimmed.includes("S.M.A.R.T. Data for")) {
                        const match = trimmed.match(/S\.M\.A\.R\.T\. Data for (\/dev\/\w+)/);
                        if (match) {
                            if (currentDevice) {
                                result[currentDevice] = deviceData;
                            }
                            currentDevice = match[1];
                            deviceData = {
                                "device": currentDevice,
                                "information": {},
                                "health": {},
                                "smartAttributes": {},
                                "powerStates": [],
                                "supportedLbaSizes": [],
                                "errorLog": [],
                                "temperatureSensors": {},
                                "allData": {},
                            };
                            inInformationSection = false;
                            inSmartDataSection = false;
                            inPowerStates = false;
                            inLbaSizes = false;
                            inErrorLog = false;
                        }
                    } else if (trimmed.includes("=== START OF INFORMATION SECTION ===")) {
                        inInformationSection = true; inSmartDataSection = false; inPowerStates = false; inLbaSizes = false; inErrorLog = false;
                    } else if (trimmed.includes("=== START OF SMART DATA SECTION ===")) {
                        inInformationSection = false; inSmartDataSection = true; inPowerStates = false; inLbaSizes = false; inErrorLog = false;
                    } else if (trimmed.includes("Supported Power States")) {
                        inPowerStates = true; inInformationSection = false; inSmartDataSection = false;
                    } else if (trimmed.includes("Supported LBA Sizes (NSID")) {
                        inLbaSizes = true; inPowerStates = false;
                    } else if (trimmed.includes("Error Information (NVMe Log")) {
                        inErrorLog = true; inLbaSizes = false;
                    } else if (trimmed.includes("===")) {
                        inInformationSection = false; inSmartDataSection = false; inPowerStates = false; inLbaSizes = false; inErrorLog = false;
                    }
                    if (trimmed.includes(":") && !inPowerStates && !inLbaSizes) {
                        const [key, ...valueParts] = trimmed.split(":");
                        const cleanKey = key.trim();
                        const value = valueParts.join(":").trim();
                        deviceData.allData[cleanKey] = value;
                        if (inInformationSection) {
                            deviceData.information[cleanKey] = value;
                        } else if (inSmartDataSection) {
                            deviceData.smartAttributes[cleanKey] = value;
                            if (cleanKey.includes("SMART overall-health")) { deviceData.health.overallStatus = value; }
                            else if (cleanKey === "Temperature") { deviceData.health.temperature = value; }
                            else if (cleanKey === "Available Spare") { deviceData.health.availableSpare = value; }
                            else if (cleanKey === "Available Spare Threshold") { deviceData.health.availableSpareThreshold = value; }
                            else if (cleanKey === "Percentage Used") { deviceData.health.percentageUsed = value; }
                            else if (cleanKey === "Power On Hours") { deviceData.health.powerOnHours = value; }
                            else if (cleanKey === "Power Cycles") { deviceData.health.powerCycles = value; }
                            else if (cleanKey === "Data Units Read") { deviceData.health.dataUnitsRead = value; }
                            else if (cleanKey === "Data Units Written") { deviceData.health.dataUnitsWritten = value; }
                            else if (cleanKey === "Host Read Commands") { deviceData.health.hostReadCommands = value; }
                            else if (cleanKey === "Host Write Commands") { deviceData.health.hostWriteCommands = value; }
                            else if (cleanKey === "Unsafe Shutdowns") { deviceData.health.unsafeShutdowns = value; }
                            else if (cleanKey === "Media and Data Integrity Errors") { deviceData.health.mediaErrors = value; }
                            else if (cleanKey === "Critical Warning") { deviceData.health.criticalWarning = value; }
                            else if (cleanKey.includes("Temperature Sensor")) { deviceData.temperatureSensors[cleanKey] = value; }
                        }
                    } else if (inPowerStates && trimmed.match(/^\d+\s+[+-]/)) {
                        const parts = trimmed.split(/\s+/).filter((part) => { return part.trim(); });
                        if (parts.length >= 10) {
                            deviceData.powerStates.push({
                                "state": parts[0],
                                "operation": parts[1],
                                "maxPower": parts[2],
                                "active": parts[3],
                                "idle": parts[4],
                                "rl": parts[5],
                                "rt": parts[6],
                                "wl": parts[7],
                                "wt": parts[8],
                                "entryLatency": parts[9],
                                "exitLatency": parts[10] || "",
                            });
                        }
                    } else if (inLbaSizes && trimmed.match(/^\d+\s+[+-]/)) {
                        const parts = trimmed.split(/\s+/).filter((part) => { return part.trim(); });
                        if (parts.length >= 4) {
                            deviceData.supportedLbaSizes.push({
                                "id": parts[0],
                                "format": parts[1],
                                "data": parts[2],
                                "metadata": parts[3],
                                "relativePerformance": parts[4] || "",
                            });
                        }
                    } else if (inErrorLog && trimmed && trimmed !== "No Errors Logged" && !trimmed.includes("Error Information")) {
                        deviceData.errorLog.push(trimmed);
                    }
                });
                if (currentDevice) {
                    result[currentDevice] = deviceData;
                }
                return result;
            };
            const parsed = parseSmart(output);
            setSmartData(parsed);
            return { "type": "printSmartData", "value": output, parsed };
        }),
    ]).then((results) => {
        const allParsedData = {};
        results.forEach((result) => {
            if (result.status === "fulfilled") {
                allParsedData[result.value.type] = result.value.parsed;
            }
        });
        setParsedData(allParsedData);
        console.log("Parsed Storage:", allParsedData);
    });
};

export { getHardwareData, getLogs, getOsInformation, getPackageManagers, getStorageInfo };

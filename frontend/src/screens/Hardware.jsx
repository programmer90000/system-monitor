import React, { useEffect, useRef } from "react";
import { runCommand } from "../lib/run-commands.js";

const Hardware = () => {
    const hasRunRef = useRef(false);

    useEffect(() => {
        if (!hasRunRef.current) {
            hasRunRef.current = true;

            console.log("Starting hardware data collection...\n");

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
        }
    }, []);

    return (
        <div/>
    );
};

export default Hardware;

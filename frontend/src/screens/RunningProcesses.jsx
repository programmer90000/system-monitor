import React, { useState, useEffect, useRef } from "react";
import { runCommand } from "../lib/run-commands.js";

const RunningProcesses = () => {
    const [runningProcesses, setRunningProcesses] = useState("");

    const hasRunRef = useRef(false);

    function parseRunningProcesses(logs) {
        const result = {
            "processTree": [],
            "totalProcesses": 0,
            "detailedFileNetworkInfo": [],
        };

        if (!logs.runningProcesses) {
            return result;
        }

        const processText = logs.runningProcesses.replace(/^"|\s*"$/g, "").trim();
        const sections = processText.split("=== DETAILED FILE AND NETWORK INFO FOR HIGH-RESOURCE PROCESSES ===");

        // Parse process tree hierarchy
        if (sections[0]) {
            const treeSection = sections[0].split("Total processes:")[0];
            const processLines = treeSection.split("\n").filter((line) => { return line.trim(); });

            // Skip header lines
            let startIndex = 0;
            for (let i = 0; i < processLines.length; i++) {
                if (processLines[i].includes("PID (PPID)")) {
                    startIndex = i + 2; // Skip header and separator line
                    break;
                }
            }

            // Parse process entries
            const processes = [];
            let currentIndent = 0;
            const processStack = [];

            for (let i = startIndex; i < processLines.length; i++) {
                const line = processLines[i].trim();
                if (!line) { continue; }

                // Calculate indentation level
                const indentMatch = processLines[i].match(/^(\s*)/);
                const indentLevel = indentMatch ? Math.floor(indentMatch[1].length / 4) : 0;

                // Parse process line
                const processMatch = line.match(/(└──\s*)?(\d+)\s+\((\d+)\s*\)\s+([\d.]+)%\s+([\d.]+)%\s+(\d+)\s+(\d+)\s+(\d+)\s+([A-Z]\s*\([^)]+\))\s+(.+)/);

                if (processMatch) {
                    const process = {
                        "pid": parseInt(processMatch[2]),
                        "ppid": parseInt(processMatch[3]),
                        "cpuPercent": parseFloat(processMatch[4]),
                        "ramPercent": parseFloat(processMatch[5]),
                        "ramKb": parseInt(processMatch[6]),
                        "files": parseInt(processMatch[7]),
                        "sockets": parseInt(processMatch[8]),
                        "state": processMatch[9].trim(),
                        "command": processMatch[10].trim(),
                        "children": [],
                        "indentLevel": indentLevel,
                    };

                    // Handle hierarchy
                    if (indentLevel > currentIndent) {
                    // Add as child of previous process
                        if (processStack.length > 0) {
                            processStack[processStack.length - 1].children.push(process);
                        }
                    } else {
                    // Pop stack until we reach the correct level
                        while (processStack.length > 0 && processStack[processStack.length - 1].indentLevel >= indentLevel) {
                            processStack.pop();
                        }

                        if (processStack.length > 0) {
                            processStack[processStack.length - 1].children.push(process);
                        } else {
                            processes.push(process);
                        }
                    }

                    processStack.push(process);
                    currentIndent = indentLevel;
                }
            }

            result.processTree = processes;

            // Parse total processes count
            const totalMatch = sections[0].match(/Total processes:\s*(\d+)/);
            if (totalMatch) {
                result.totalProcesses = parseInt(totalMatch[1]);
            }
        }

        // Parse detailed file and network info
        if (sections[1]) {
            const detailedSections = sections[1].split("--- PID");
            for (let i = 1; i < detailedSections.length; i++) {
                const section = detailedSections[i].trim();
                const pidMatch = section.match(/^(\d+):\s*([^(\n]+)\s*\([^)]+\)\s*---/);

                if (pidMatch) {
                    const processInfo = {
                        "pid": parseInt(pidMatch[1]),
                        "command": pidMatch[2].trim(),
                        "openFiles": [],
                        "networkConnections": [],
                    };

                    // Parse CPU and RAM info from the header
                    const headerMatch = section.match(/\(CPU:\s*([\d.]+)%\s*,\s*RAM:\s*([\d.]+)%\s*,\s*Files:\s*(\d+)\s*,\s*Sockets:\s*(\d+)\)/);
                    if (headerMatch) {
                        processInfo.cpuPercent = parseFloat(headerMatch[1]);
                        processInfo.ramPercent = parseFloat(headerMatch[2]);
                        processInfo.totalFiles = parseInt(headerMatch[3]);
                        processInfo.totalSockets = parseInt(headerMatch[4]);
                    }

                    // Parse open files
                    const filesSection = section.match(/Open files \(first \d+\):([\s\S]*?)(?=Network connections:|$)/);
                    if (filesSection) {
                        const fileLines = filesSection[1].split("\n").filter((line) => { return line.trim(); });
                        for (const line of fileLines) {
                            const fileMatch = line.match(/\s*FD\s+(\d+)\s*->\s*(.+)/);
                            if (fileMatch) {
                                processInfo.openFiles.push({
                                    "fd": parseInt(fileMatch[1]),
                                    "path": fileMatch[2].trim(),
                                });
                            }
                        }
                    }

                    // Parse network connections
                    const networkSection = section.match(/Network connections:\s*([\s\S]*?)(?=--- PID|\s*$)/);
                    if (networkSection && networkSection[1].trim()) {
                        const connectionLines = networkSection[1].split("\n").filter((line) => { return line.trim(); });
                        for (const line of connectionLines) {
                            if (line.trim()) {
                                processInfo.networkConnections.push(line.trim());
                            }
                        }
                    }

                    result.detailedFileNetworkInfo.push(processInfo);
                }
            }
        }

        return result;
    }

    useEffect(() => {
        if (!hasRunRef.current) {
            hasRunRef.current = true;

            Promise.allSettled([
                runCommand("display_running_processes", []).then((output) => {
                    setRunningProcesses(output);
                    const parsedData = parseRunningProcesses({ "runningProcesses": output });
                    console.log("Parsed Running Processes:", parsedData);
                
                    return { "type": "runningProcesses", "value": output };
                }),                
            ]).then((results) => {
                results.forEach((result, index) => {
                    if (result.status === "fulfilled") {
                    }
                    if (result.status === "rejected") {
                        console.error(`Command ${index} failed:`, result.reason);
                    }
                });
            });
        }
    }, []);

    return (
        <div>
            <p>Test</p>
        </div>
    );
};

export default RunningProcesses;

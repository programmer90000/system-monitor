import React, { useState, useEffect, useRef } from "react";
import { runCommand, runSudoCommand } from "../lib/run-commands.js";

const Storage = () => {
    const [storageDevices, setStorageDevices] = useState([]);
    const [storageDevicesWithTemperatureReporting, setStorageDevicesWithTemperatureReporting] = useState([]);
    const [smartData, setSmartData] = useState({});
    const [parsedData, setParsedData] = useState(null);

    const hasRunRef = useRef(false);

    function parseStorageDevices(output) {
        const devices = [];
        const lines = output.split("\n");

        lines.forEach((line) => {
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
    }

    function parseTemperatureDevices(output) {
        const devices = [];
        const lines = output.split("\n");

        lines.forEach((line) => {
            const trimmed = line.trim();
            if (trimmed.startsWith("Storage Device Name:")) {
                const match = trimmed.match(/Storage Device Name: (\w+) Temperature: ([\d.]+)°C/);
                if (match) {
                    devices.push({ "name": match[1], "temperature": parseFloat(match[2]), "temperatureUnit": "°C" });
                }
            }
        });

        return devices;
    }

    function parseSmartData(output) {
        const result = {};
        const lines = output.split("\n");

        let currentDevice = "";
        let deviceData = {};
        let inInformationSection = false;
        let inSmartDataSection = false;
        let inPowerStates = false;
        let inLbaSizes = false;
        let inErrorLog = false;

        lines.forEach((line) => {
            const trimmed = line.trim();
            
            // Detect device
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
                        "allData": {}, // Store all key-value pairs
                    };
                    inInformationSection = false;
                    inSmartDataSection = false;
                    inPowerStates = false;
                    inLbaSizes = false;
                    inErrorLog = false;
                }
            }

            // Section detection
            else if (trimmed.includes("=== START OF INFORMATION SECTION ===")) {
                inInformationSection = true;
                inSmartDataSection = false;
                inPowerStates = false;
                inLbaSizes = false;
                inErrorLog = false;
            }
            else if (trimmed.includes("=== START OF SMART DATA SECTION ===")) {
                inInformationSection = false;
                inSmartDataSection = true;
                inPowerStates = false;
                inLbaSizes = false;
                inErrorLog = false;
            }
            else if (trimmed.includes("Supported Power States")) {
                inPowerStates = true;
                inInformationSection = false;
                inSmartDataSection = false;
            }
            else if (trimmed.includes("Supported LBA Sizes (NSID")) {
                inLbaSizes = true;
                inPowerStates = false;
            }
            else if (trimmed.includes("Error Information (NVMe Log")) {
                inErrorLog = true;
                inLbaSizes = false;
            }
            else if (trimmed.includes("===")) {
                // Section boundaries
                inInformationSection = false;
                inSmartDataSection = false;
                inPowerStates = false;
                inLbaSizes = false;
                inErrorLog = false;
            }

            // Parse all key-value pairs regardless of section
            if (trimmed.includes(":") && !inPowerStates && !inLbaSizes) {
                const [key, ...valueParts] = trimmed.split(":");
                const cleanKey = key.trim();
                const value = valueParts.join(":").trim();

                // Store in allData for complete record
                deviceData.allData[cleanKey] = value;

                if (inInformationSection) {
                    deviceData.information[cleanKey] = value;
                } else if (inSmartDataSection) {
                    deviceData.smartAttributes[cleanKey] = value;

                    // Parse specific health metrics
                    if (cleanKey.includes("SMART overall-health")) {
                        deviceData.health.overallStatus = value;
                    } else if (cleanKey === "Temperature") {
                        deviceData.health.temperature = value;
                    } else if (cleanKey === "Available Spare") {
                        deviceData.health.availableSpare = value;
                    } else if (cleanKey === "Available Spare Threshold") {
                        deviceData.health.availableSpareThreshold = value;
                    } else if (cleanKey === "Percentage Used") {
                        deviceData.health.percentageUsed = value;
                    } else if (cleanKey === "Power On Hours") {
                        deviceData.health.powerOnHours = value;
                    } else if (cleanKey === "Power Cycles") {
                        deviceData.health.powerCycles = value;
                    } else if (cleanKey === "Data Units Read") {
                        deviceData.health.dataUnitsRead = value;
                    } else if (cleanKey === "Data Units Written") {
                        deviceData.health.dataUnitsWritten = value;
                    } else if (cleanKey === "Host Read Commands") {
                        deviceData.health.hostReadCommands = value;
                    } else if (cleanKey === "Host Write Commands") {
                        deviceData.health.hostWriteCommands = value;
                    } else if (cleanKey === "Unsafe Shutdowns") {
                        deviceData.health.unsafeShutdowns = value;
                    } else if (cleanKey === "Media and Data Integrity Errors") {
                        deviceData.health.mediaErrors = value;
                    } else if (cleanKey === "Critical Warning") {
                        deviceData.health.criticalWarning = value;
                    } else if (cleanKey.includes("Temperature Sensor")) {
                        deviceData.temperatureSensors[cleanKey] = value;
                    }
                }
            }

            // Parse power states table
            else if (inPowerStates && trimmed.match(/^\d+\s+[+-]/)) {
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
            }

            // Parse LBA sizes table
            else if (inLbaSizes && trimmed.match(/^\d+\s+[+-]/)) {
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
            }

            // Parse error log
            else if (inErrorLog && trimmed && trimmed !== "No Errors Logged" && !trimmed.includes("Error Information")) {
                deviceData.errorLog.push(trimmed);
            }
        });

        // Add the last device
        if (currentDevice) {
            result[currentDevice] = deviceData;
        }

        return result;
    }

    useEffect(() => {
        if (!hasRunRef.current) {
            hasRunRef.current = true;

            Promise.allSettled([
                runCommand("detect_all_storage_devices", []).then((output) => {
                    const parsed = parseStorageDevices(output);
                    setStorageDevices(parsed);
                    return { "type": "detectAllStorageDevices", "value": output, "parsed": parsed };
                }),

                runCommand("find_storage_devices_with_temperature_reporting", []).then((output) => {
                    const parsed = parseTemperatureDevices(output);
                    setStorageDevicesWithTemperatureReporting(parsed);
                    return { "type": "findStorageDevicesWithTemperatureReporting", "value": output, "parsed": parsed };
                }),

                runSudoCommand("print_smart_data", []).then((output) => {
                    const parsed = parseSmartData(output);
                    setSmartData(parsed);
                    return { "type": "printSmartData", "value": output, "parsed": parsed };
                }),
            ]).then((results) => {
                const allParsedData = {};

                results.forEach((result, index) => {
                    if (result.status === "fulfilled") {
                        console.log(`${result.value.type}:`, result.value.parsed);
                        allParsedData[result.value.type] = result.value.parsed;
                    }
                    if (result.status === "rejected") {
                        console.error(`Command ${index} failed:`, result.reason);
                    }
                });

                setParsedData(allParsedData);
                console.log(allParsedData);
            });
        }
    }, []);

    return (
        <div>
            <p>Test</p>
        </div>
    );
};

export default Storage;

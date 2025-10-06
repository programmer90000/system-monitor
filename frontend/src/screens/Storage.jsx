import React, { useState, useEffect, useRef } from "react";
import { runCommand, runSudoCommand } from "../lib/run-commands.js";

const Storage = () => {
    const [storageDevices, setStorageDevices] = useState("");
    const [storageDevicesWithTemperaureReporting, setStorageDevicesWithTemperaureReporting] = useState("");
    const [smartData, setSmartData] = useState("");

    const hasRunRef = useRef(false);

    useEffect(() => {
        if (!hasRunRef.current) {
            hasRunRef.current = true;

            Promise.allSettled([
                runCommand("detect_all_storage_devices", []).then((output) => {
                    setStorageDevices(output);
                    return { "type": "detectAllStorageDevices", "value": output };
                }),

                runCommand("find_storage_devices_with_temperature_reporting", []).then((output) => {
                    setStorageDevicesWithTemperaureReporting(output);
                    return { "type": "findStorageDevicesWithTemperatureReporting", "value": output };
                }),

                runSudoCommand("print_smart_data", []).then((output) => {
                    setSmartData(output);
                    return { "type": "printSmartData", "value": output };
                }),
            ]).then((results) => {
                results.forEach((result, index) => {
                    if (result.status === "fulfilled") {
                        console.log(`${result.value.type}:`, result.value.value);
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

export default Storage;

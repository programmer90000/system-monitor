import React, { useState, useEffect, useRef } from "react";
import { invoke } from "@tauri-apps/api/core";

const Storage = () => {
    const [systemInfo, setSystemInfo] = useState({
        "allStorageDevices": "",
        "storageDevicesWithTemperaure": "",
        // ! "smartData": "", requires root privileges
    });

    const hasRunRef = useRef(false);

    async function runCProgram() {
        const all_storage_devices = await invoke("run_c_program", { "function": "detect_all_storage_devices" });
        const storage_devices_with_temperature = await invoke("run_c_program", { "function": "find_storage_devices_with_temperature_reporting" });
        // ! const smart_data = await invoke("run_c_program", { "function": "print_smart_data" }); requires root privileges

        setSystemInfo({
            "allStorageDevices": all_storage_devices,
            "storageDevicesWithTemperaure": storage_devices_with_temperature,
            // ! "smartData": smart_data, requires root privileges
        });
    }

    useEffect(() => {
        if (!hasRunRef.current) {
            hasRunRef.current = true;
            runCProgram();
        }
    }, []);

    useEffect(() => {
        if (!systemInfo) { return; }

        // All system info blocks
        const allBlocks = [
            systemInfo.allStorageDevices,
            systemInfo.storageDevicesWithTemperaure,
            systemInfo.smartData,
        ];

        // Keep allValues the same (values only)
        const allValues = allBlocks.flatMap((block) =>
        { return (block || "")
            .split("\n")
            .map((line) => { return line.trim(); })
            .map((line) => {
                if (!line || (/^={3,}/).test(line) || (/^-{3,}/).test(line)) { return null; }
                const match = line.match(/^[^=:]+[=:]\s*(.*)$/);
                return match ? match[1].replace(/^"+|"+$/g, "").trim() : line;
            })
            .filter(Boolean); },
        );

        // Log with simple sequential keys
        allValues.forEach((value) => {
            console.log(value);
        });

    }, [systemInfo]);

    return (
        <div>
            <p>Test</p>
        </div>
    );
};

export default Storage;

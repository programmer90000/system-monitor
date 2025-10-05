import React, { useState, useEffect, useRef } from "react";
import { invoke } from "@tauri-apps/api/core";

const Temperature = () => {
    const [systemInfo, setSystemInfo] = useState({
        "cpuTemperature": "",
        "gpuTemperature": "",
        "vrmTemperature": "",
        "chipsetTemperature": "",
        "motherboardTemperature": "",
        "psuTemperature": "",
        "caseTemperature": "",
    });

    const hasRunRef = useRef(false);

    async function runCProgram() {
        let cpu_temperature = "";
        let gpu_temperature = "";
        let vrm_temperature = "";
        let chipset_temperature = "";
        let motherboard_temperature = "";
        let psu_temperature = "";
        let case_temperature = "";

        try {
            cpu_temperature = await invoke("run_c_program", { "function": "get_cpu_temperature" });
            console.log("CPU Temperature - Data Loaded");
        } catch (error) {
            console.error("Error fetching CPU temperature:", error);
            cpu_temperature = "Error: Failed to fetch CPU temperature";
        }

        try {
            gpu_temperature = await invoke("run_c_program", { "function": "get_gpu_temperature" });
            console.log("GPU Temperature - Data Loaded");
        } catch (error) {
            console.error("Error fetching GPU temperature:", error);
            gpu_temperature = "Error: Failed to fetch GPU temperature";
        }

        try {
            vrm_temperature = await invoke("run_c_program", { "function": "get_vrm_temperature" });
            console.log("VRM Temperature - Data Loaded");
        } catch (error) {
            console.error("Error fetching VRM temperature:", error);
            vrm_temperature = "Error: Failed to fetch VRM temperature";
        }

        try {
            chipset_temperature = await invoke("run_c_program", { "function": "get_chipset_temperature" });
            console.log("Chipset Temperature - Data Loaded");
        } catch (error) {
            console.error("Error fetching chipset temperature:", error);
            chipset_temperature = "Error: Failed to fetch chipset temperature";
        }

        try {
            motherboard_temperature = await invoke("run_c_program", { "function": "get_motherboard_temperature" });
            console.log("Motherboard Temperature - Data Loaded");
        } catch (error) {
            console.error("Error fetching motherboard temperature:", error);
            motherboard_temperature = "Error: Failed to fetch motherboard temperature";
        }

        try {
            psu_temperature = await invoke("run_c_program", { "function": "get_psu_temperature" });
            console.log("PSU Temperature - Data Loaded");
        } catch (error) {
            console.error("Error fetching PSU temperature:", error);
            psu_temperature = "Error: Failed to fetch PSU temperature";
        }

        try {
            case_temperature = await invoke("run_c_program", { "function": "get_case_temperature" });
            console.log("Case Temperature - Data Loaded");
        } catch (error) {
            console.error("Error fetching case temperature:", error);
            case_temperature = "Error: Failed to fetch case temperature";
        }

        setSystemInfo({
            "cpuTemperature": cpu_temperature,
            "gpuTemperature": gpu_temperature,
            "vrmTemperature": vrm_temperature,
            "chipsetTemperature": chipset_temperature,
            "motherboardTemperature": motherboard_temperature,
            "psuTemperature": psu_temperature,
            "caseTemperature": case_temperature,
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
            systemInfo.cpuTemperature,
            systemInfo.gpuTemperature,
            systemInfo.vrmTemperature,
            systemInfo.chipsetTemperature,
            systemInfo.motherboardTemperature,
            systemInfo.psuTemperature,
            systemInfo.caseTemperature,
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

export default Temperature;

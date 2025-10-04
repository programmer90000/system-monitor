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
        const cpu_temperature = await invoke("run_c_program", { "function": "get_cpu_temperature" });
        const gpu_temperature = await invoke("run_c_program", { "function": "get_gpu_temperature" });
        const vrm_temperature = await invoke("run_c_program", { "function": "get_vrm_temperature" });
        const chipset_temperature = await invoke("run_c_program", { "function": "get_chipset_temperature" });
        const motherboard_temperature = await invoke("run_c_program", { "function": "get_motherboard_temperature" });
        const psu_temperature = await invoke("run_c_program", { "function": "get_psu_temperature" });
        const case_temperature = await invoke("run_c_program", { "function": "get_case_temperature" });

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

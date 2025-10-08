import React, { useState, useEffect, useRef } from "react";
import { runCommand } from "../lib/run-commands.js";

const Temperature = () => {
    const [temperatures, setTemperatures] = useState({
        "cpuTemperature": "",
        "gpuTemperature": "",
        "vrmTemperature": "",
        "chipsetTemperature": "",
        "motherboardTemperature": "",
        "psuTemperature": "",
        "caseTemperature": "",
    });
    const [parsedData, setParsedData] = useState(null);

    const hasRunRef = useRef(false);

    function parseTemperatures(logs) {
        const result = {};

        if (logs.cpuTemperature) {
            const cpuTempText = logs.cpuTemperature.replace(/^"|\s*"$/g, "").trim();
            result.cpuTemperature = cpuTempText.match(/CPU Temperature:\s*([\d.]+)°C/)?.[1] || "";
        }

        if (logs.gpuTemperature) {
            const gpuTempText = logs.gpuTemperature.replace(/^"|\s*"$/g, "").trim();
            result.gpuTemperature = gpuTempText.match(/GPU Temperature:\s*([\d.]+)°C/)?.[1] || "";
        }

        if (logs.vrmTemperature) {
            const vrmTempText = logs.vrmTemperature.replace(/^"|\s*"$/g, "").trim();
            result.vrmTemperature = vrmTempText.match(/VRM Temperature:\s*([\d.]+)°C/)?.[1] || "";
        }

        if (logs.chipsetTemperature) {
            const chipsetTempText = logs.chipsetTemperature.replace(/^"|\s*"$/g, "").trim();
            result.chipsetTemperature = chipsetTempText.match(/Chipset Temperature:\s*([\d.]+)°C/)?.[1] || "";
        }

        if (logs.motherboardTemperature) {
            const motherboardTempText = logs.motherboardTemperature.replace(/^"|\s*"$/g, "").trim();
            result.motherboardTemperature = motherboardTempText.match(/Motherboard Temperature:\s*([\d.]+)°C/)?.[1] || "";
        }

        if (logs.psuTemperature) {
            const psuTempText = logs.psuTemperature.replace(/^"|\s*"$/g, "").trim();
            result.psuTemperature = psuTempText.match(/PSU Temperature:\s*([\d.]+)°C/)?.[1] || "";
        }

        if (logs.caseTemperature) {
            const caseTempText = logs.caseTemperature.replace(/^"|\s*"$/g, "").trim();
            result.caseTemperature = caseTempText.match(/Case Temperature:\s*([\d.]+)°C/)?.[1] || "";
        }

        return result;
    }

    useEffect(() => {
        if (!hasRunRef.current) {
            hasRunRef.current = true;

            Promise.allSettled([
                runCommand("get_cpu_temperature", []).then((output) => {
                    setTemperatures((prev) => { return { ...prev, "cpuTemperature": output }; });
                    return { "type": "cpuTemperature", "value": output };
                }),

                runCommand("get_gpu_temperature", []).then((output) => {
                    setTemperatures((prev) => { return { ...prev, "gpuTemperature": output }; });
                    return { "type": "gpuTemperature", "value": output };
                }),

                runCommand("get_vrm_temperature", []).then((output) => {
                    setTemperatures((prev) => { return { ...prev, "vrmTemperature": output }; });
                    return { "type": "vrmTemperature", "value": output };
                }),

                runCommand("get_chipset_temperature", []).then((output) => {
                    setTemperatures((prev) => { return { ...prev, "chipsetTemperature": output }; });
                    return { "type": "chipsetTemperature", "value": output };
                }),

                runCommand("get_motherboard_temperature", []).then((output) => {
                    setTemperatures((prev) => { return { ...prev, "motherboardTemperature": output }; });
                    return { "type": "motherboardTemperature", "value": output };
                }),

                runCommand("get_psu_temperature", []).then((output) => {
                    setTemperatures((prev) => { return { ...prev, "psuTemperature": output }; });
                    return { "type": "psuTemperature", "value": output };
                }),                
                         
                runCommand("get_case_temperature", []).then((output) => {
                    setTemperatures((prev) => { return { ...prev, "caseTemperature": output }; });
                    return { "type": "caseTemperature", "value": output };
                }),
                
            ]).then((results) => {
                const allData = {};
                results.forEach((result) => {
                    if (result.status === "fulfilled") {
                        allData[result.value.type] = result.value.value;
                        console.log(`${result.value.type}:`, result.value.value);
                    }
                    if (result.status === "rejected") {
                        console.error("Command failed:", result.reason);
                    }
                });

                const parsed = parseTemperatures(allData);
                setParsedData(parsed);
                console.log("Parsed Temperatures:", parsed);
            });
        }
    }, []);

    return (
        <div>
            <p>Test</p>
        </div>
    );
};

export default Temperature;

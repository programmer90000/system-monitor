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

    const hasRunRef = useRef(false);

    useEffect(() => {
        if (!hasRunRef.current) {
            hasRunRef.current = true;

            Promise.allSettled([
                runCommand("get_cpu_temperature", []).then((output) => {
                    setTemperatures((prev) => { return { ...prev, "cpuTemperature": output }; });
                    return { "type": "cpu", "value": output };
                }),

                runCommand("get_gpu_temperature", []).then((output) => {
                    setTemperatures((prev) => { return { ...prev, "gpuTemperature": output }; });
                    return { "type": "gpu", "value": output };
                }),

                runCommand("get_vrm_temperature", []).then((output) => {
                    setTemperatures((prev) => { return { ...prev, "vrmTemperature": output }; });
                    return { "type": "vrm", "value": output };
                }),

                runCommand("get_chipset_temperature", []).then((output) => {
                    setTemperatures((prev) => { return { ...prev, "chipsetTemperature": output }; });
                    return { "type": "chipset", "value": output };
                }),

                runCommand("get_motherboard_temperature", []).then((output) => {
                    setTemperatures((prev) => { return { ...prev, "chipsetTemperature": output }; });
                    return { "type": "chipset", "value": output };
                }),

                runCommand("get_psu_temperature", []).then((output) => {
                    setTemperatures((prev) => { return { ...prev, "chipsetTemperature": output }; });
                    return { "type": "chipset", "value": output };
                }),                
                         
                runCommand("get_case_temperature", []).then((output) => {
                    setTemperatures((prev) => { return { ...prev, "chipsetTemperature": output }; });
                    return { "type": "chipset", "value": output };
                }),
                
            ]).then((results) => {
                results.forEach((result, index) => {
                    if (result.status === "fulfilled") {
                        console.log(`${result.value.type} temperature:`, result.value.value);
                    }
                    if (result.status === "rejected") {
                        console.error(`Command ${index} failed:`, result.reason);
                    }
                });
                
                // Optional: Log the complete temperatures state
                console.log("All temperatures:", temperatures);
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

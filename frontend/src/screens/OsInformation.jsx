import React, { useState, useEffect, useRef } from "react";
import { invoke } from "@tauri-apps/api/core";

const OsInformation = () => {
    const [systemInfo, setSystemInfo] = useState({
        "osInfo": "",
        "distributionInfo": "",
        "kernelDetails": "",
        "libraryVersions": "",
        "securityInfo": "",
        "systemLimits": "",
        "unameInfo": "",
    });

    const hasRunRef = useRef(false);

    async function runCProgram() {
        const os_info = await invoke("run_c_program", { "function": "print_detailed_os_info" });
        const distribution_info = await invoke("run_c_program", { "function": "print_distribution_info" });
        const kernel_details = await invoke("run_c_program", { "function": "print_kernel_details" });
        const library_versions = await invoke("run_c_program", { "function": "print_library_versions" });
        const security_info = await invoke("run_c_program", { "function": "print_security_info" });
        const system_limits = await invoke("run_c_program", { "function": "print_system_limits" });
        const uname_info = await invoke("run_c_program", { "function": "print_uname_info" });

        setSystemInfo({
            "osInfo": os_info,
            "distributionInfo": distribution_info,
            "kernelDetails": kernel_details,
            "libraryVersions": library_versions,
            "securityInfo": security_info,
            "systemLimits": system_limits,
            "unameInfo": uname_info,
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
            systemInfo.osInfo,
            systemInfo.distributionInfo,
            systemInfo.kernelDetails,
            systemInfo.libraryVersions,
            systemInfo.securityInfo,
            systemInfo.systemLimits,
            systemInfo.unameInfo,
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

export default OsInformation;

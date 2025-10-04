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
        console.log(`
osInfo: ${systemInfo.osInfo}
distributionInfo: ${systemInfo.distributionInfo}
kernelDetails: ${systemInfo.kernelDetails}
libraryVersions: ${systemInfo.libraryVersions}
securityInfo: ${systemInfo.securityInfo}
systemLimits: ${systemInfo.systemLimits}
unameInfo: ${systemInfo.unameInfo}
            `);
    }, [systemInfo]);

    return (
        <div>
            <p>Test</p>
        </div>
    );
};

export default OsInformation;

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
        let os_info = "";
        let distribution_info = "";
        let kernel_details = "";
        let library_versions = "";
        let security_info = "";
        let system_limits = "";
        let uname_info = "";

        try {
            os_info = await invoke("run_c_program", { "function": "print_detailed_os_info" });
            console.log("OS info - Data Loaded");
        } catch (error) {
            console.error("Error fetching OS info:", error);
            os_info = "Error: Failed to fetch OS information";
        }

        try {
            distribution_info = await invoke("run_c_program", { "function": "print_distribution_info" });
            console.log("Distribution info - Data Loaded");
        } catch (error) {
            console.error("Error fetching distribution info:", error);
            distribution_info = "Error: Failed to fetch distribution information";
        }

        try {
            kernel_details = await invoke("run_c_program", { "function": "print_kernel_details" });
            console.log("Kernel info - Data Loaded");
        } catch (error) {
            console.error("Error fetching kernel details:", error);
            kernel_details = "Error: Failed to fetch kernel details";
        }

        try {
            library_versions = await invoke("run_c_program", { "function": "print_library_versions" });
            console.log("Library versions - Data Loaded");
        } catch (error) {
            console.error("Error fetching library versions:", error);
            library_versions = "Error: Failed to fetch library versions";
        }

        try {
            security_info = await invoke("run_c_program", { "function": "print_security_info" });
            console.log("Security info - Data Loaded");
        } catch (error) {
            console.error("Error fetching security info:", error);
            security_info = "Error: Failed to fetch security information";
        }

        try {
            system_limits = await invoke("run_c_program", { "function": "print_system_limits" });
            console.log("System Limits info - Data Loaded");
        } catch (error) {
            console.error("Error fetching system limits:", error);
            system_limits = "Error: Failed to fetch system limits";
        }

        try {
            uname_info = await invoke("run_c_program", { "function": "print_uname_info" });
            console.log("Uname info - Data Loaded");
        } catch (error) {
            console.error("Error fetching uname info:", error);
            uname_info = "Error: Failed to fetch uname information";
        }

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

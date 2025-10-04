import React, { useState, useEffect, useRef } from "react";
import { invoke } from "@tauri-apps/api/core";

const OsInformation = () => {
    const [osSummary, setOsSummary] = useState("");
    const [osInfo, setOsInfo] = useState("");
    const [distributionInfo, setDistributionInfo] = useState("");
    const [kernelDetails, setKernelDetails] = useState("");
    const [libraryVersions, setLibraryVersions] = useState("");
    const [securityInfo, setSecurityInfo] = useState("");
    const [systemLimits, setSystemLimits] = useState("");
    const [unameInfo, setUnameInfo] = useState("");

    const hasRunRef = useRef(false);

    async function runCProgram() {
        const os_summary = await invoke("run_c_program", { "function": "print_os_summary" });
        const os_info = await invoke("run_c_program", { "function": "print_detailed_os_info" });
        const distribution_info = await invoke("run_c_program", { "function": "print_distribution_info" });
        const kernel_details = await invoke("run_c_program", { "function": "print_kernel_details" });
        const library_versions = await invoke("run_c_program", { "function": "print_library_versions" });
        const security_info = await invoke("run_c_program", { "function": "print_security_info" });
        const system_limits = await invoke("run_c_program", { "function": "print_system_limits" });
        const uname_info = await invoke("run_c_program", { "function": "print_uname_info" });

        setOsSummary(os_summary);
        setOsInfo(os_info);
        setDistributionInfo(distribution_info);
        setKernelDetails(kernel_details);
        setLibraryVersions(library_versions);
        setSecurityInfo(security_info);
        setSystemLimits(system_limits);
        setUnameInfo(uname_info);

        console.log(osSummary);
        console.log(osInfo);
        console.log(distributionInfo);
        console.log(kernelDetails);
        console.log(libraryVersions);
        console.log(securityInfo);
        console.log(systemLimits);
        console.log(unameInfo);
    }

    useEffect(() => {
        if (!hasRunRef.current) {
            hasRunRef.current = true;
            runCProgram();
        }
    }, []);
    return (
        <div>
            <p>Test</p>
        </div>
    );
};

export default OsInformation;

import React, { useState, useEffect, useRef } from "react";
import { runCommand } from "../lib/run-commands.js";

const OsInformation = () => {
    const [osInformation, setOsInformation] = useState({
        "osInfo": "",
        "distributionInfo": "",
        "kernelDetails": "",
        "libraryVersions": "",
        "securityInfo": "",
        "systemLimits": "",
        "unameInfo": "",
    });

    const hasRunRef = useRef(false);

    useEffect(() => {
        if (!hasRunRef.current) {
            hasRunRef.current = true;

            Promise.allSettled([
                runCommand("print_detailed_os_info", []).then((output) => {
                    setOsInformation((prev) => { return { ...prev, "osInfo": output }; });
                    return { "type": "osInfo", "value": output };
                }),

                runCommand("print_distribution_info", []).then((output) => {
                    setOsInformation((prev) => { return { ...prev, "distributionInfo": output }; });
                    return { "type": "distributionInfo", "value": output };
                }),

                runCommand("print_kernel_details", []).then((output) => {
                    setOsInformation((prev) => { return { ...prev, "kernelDetails": output }; });
                    return { "type": "kernelDetails", "value": output };
                }),

                runCommand("print_library_versions", []).then((output) => {
                    setOsInformation((prev) => { return { ...prev, "libraryVersions": output }; });
                    return { "type": "libraryVersions", "value": output };
                }),

                runCommand("print_security_info", []).then((output) => {
                    setOsInformation((prev) => { return { ...prev, "securityInfo": output }; });
                    return { "type": "securityInfo", "value": output };
                }),

                runCommand("print_system_limits", []).then((output) => {
                    setOsInformation((prev) => { return { ...prev, "systemLimits": output }; });
                    return { "type": "systemLimits", "value": output };
                }),                
                         
                runCommand("print_uname_info", []).then((output) => {
                    setOsInformation((prev) => { return { ...prev, "unameInfo": output }; });
                    return { "type": "unameInfo", "value": output };
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

export default OsInformation;

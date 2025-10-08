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
    const [parsedData, setParsedData] = useState(null);

    const hasRunRef = useRef(false);

    function parseSystemInfo(logs) {
        const result = {};

        if (logs.osInfo) {
            const osInfoText = logs.osInfo.replace(/^"|\s*"$/g, "").trim();
            result.osInfo = {
                "prettyName": osInfoText.match(/PRETTY_NAME="([^"]+)"/)?.[1],
                "name": osInfoText.match(/NAME="([^"]+)"/)?.[1],
                "versionId": osInfoText.match(/VERSION_ID="([^"]+)"/)?.[1],
                "version": osInfoText.match(/VERSION="([^"]+)"/)?.[1],
                "versionCodename": osInfoText.match(/VERSION_CODENAME=([^\s]+)/)?.[1],
                "id": osInfoText.match(/ID=([^\s]+)/)?.[1],
                "homeUrl": osInfoText.match(/HOME_URL="([^"]+)"/)?.[1],
                "supportUrl": osInfoText.match(/SUPPORT_URL="([^"]+)"/)?.[1],
                "bugReportUrl": osInfoText.match(/BUG_REPORT_URL="([^"]+)"/)?.[1],
            };
        }

        if (logs.distributionInfo) {
            const distroLines = logs.distributionInfo.replace(/^"|\s*"$/g, "").trim().split("\n");
            result.distributionInfo = {
                "packageManager": distroLines[0] || "",
                "initSystem": distroLines[1] || "",
                "debianVersion": distroLines[2] || "",
                "systemdVersion": distroLines[3] || "",
            };
        }

        if (logs.kernelDetails) {
            const kernelText = logs.kernelDetails.replace(/^"|\s*"$/g, "").trim();
            result.kernelDetails = {
                "fullVersion": kernelText.match(/Full Kernel Version:\s*(.+?)(?=\n|$)/)?.[1]?.trim(),
                "commandLine": kernelText.match(/Kernel Command Line:\s*(.+?)(?=\n|$)/)?.[1]?.trim(),
                "architecture": kernelText.match(/Kernel Architecture:\s*(.+?)(?=\n|$)/)?.[1]?.trim(),
            };
        }

        if (logs.libraryVersions) {
            const libText = logs.libraryVersions.replace(/^"|\s*"$/g, "").trim();
            result.libraryVersions = {
                "glibcVersion": libText.match(/GLIBC Version:\s*([^\n]+)/)?.[1],
                "glibcRelease": libText.match(/GLIBC Release:\s*([^\n]+)/)?.[1],
                "usingGlibc": libText.match(/Using GLIBC:\s*([^\n]+)/)?.[1],
                "gccVersion": libText.match(/GCC Version:\s*([^\n]+)/)?.[1],
                "cStandard": libText.match(/C Standard:\s*([^\n]+)/)?.[1],
            };
        }

        if (logs.securityInfo) {
            const securityText = logs.securityInfo.replace(/^"|\s*"$/g, "").trim();
            result.securityInfo = {
                "securityUpdatesConfigured": securityText.match(/Security updates configured:\s*([^\n]+)/)?.[1] === "Yes",
                "securityModules": securityText.match(/Security Modules:\s*([^\n]+)/)?.[1]?.split(",") || [],
            };
        }

        if (logs.systemLimits) {
            const limitsText = logs.systemLimits.replace(/^"|\s*"$/g, "").trim();
            result.systemLimits = {
                "maxPid": parseInt(limitsText.match(/Maximum PID:\s*([^\n]+)/)?.[1]) || 0,
                "maxThreads": parseInt(limitsText.match(/Maximum threads:\s*([^\n]+)/)?.[1]) || 0,
                "maxPtys": parseInt(limitsText.match(/Maximum PTYs:\s*([^\n]+)/)?.[1]) || 0,
            };
        }

        if (logs.unameInfo) {
            const unameText = logs.unameInfo.replace(/^"|\s*"$/g, "").trim();
            result.unameInfo = {
                "os": unameText.match(/OS:\s*([^\n]+)/)?.[1],
                "hostname": unameText.match(/Hostname:\s*([^\n]+)/)?.[1],
                "kernelRelease": unameText.match(/Kernel Release:\s*([^\n]+)/)?.[1],
                "kernelVersion": unameText.match(/Kernel Version:\s*([^\n]+)/)?.[1],
                "architecture": unameText.match(/Architecture:\s*([^\n]+)/)?.[1],
            };
        }
  
        return result;
    }

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
                const allData = {};
                results.forEach((result) => {
                    if (result.status === "fulfilled") {
                        allData[result.value.type] = result.value.value;
                        console.log(`${result.value.type}:`, result.value.value);
                    }
                    if (result.status === "rejected") {
                        console.error(`Command ${index} failed:`, result.reason);
                    }
                });

                const parsed = parseSystemInfo(allData);
                setParsedData(parsed);
                console.log("Parsed System Info:", parsed);
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

import React, { useState, useEffect, useRef } from "react";
import { invoke } from "@tauri-apps/api/core";
import { runCommand } from "../lib/run-commands.js";

const PackageManagers = () => {
    const [systemInfo, setSystemInfo] = useState({
        "packageManagers": "",
    });
    const [packageManagers, setPackageManagers] = useState("");

    const hasRunRef = useRef(false);

    useEffect(() => {
        if (!hasRunRef.current) {
            hasRunRef.current = true;

            Promise.allSettled([
                runCommand("detect_all_package_managers", []).then((output) => {
                    setPackageManagers(output);
                    return output;
                }),
            ]).then((results) => {
                results.forEach((result, index) => {
                    if (result.status === "fulfilled") {
                        console.log(result.value);
                    }
                    if (result.status === "rejected") {
                        console.error(`Command ${index} failed: ${result.reason}`);
                    }
                });
            });
        }
    }, []);

    async function runCProgram() {
        let package_managers = "";

        try {
            package_managers = runCommand("detect_all_package_managers");
            console.log("Package Managers - Data Loaded");
        } catch (error) {
            console.error("Error fetching package managers:", error);
        }

        setSystemInfo({
            "packageManagers": package_managers,
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
            systemInfo.packageManagers,
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

export default PackageManagers;

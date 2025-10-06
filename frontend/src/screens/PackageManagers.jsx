import React, { useState, useEffect, useRef } from "react";
import { invoke } from "@tauri-apps/api/core";

const PackageManagers = () => {
    const [systemInfo, setSystemInfo] = useState({
        "packageManagers": "",
    });

    const hasRunRef = useRef(false);

    async function runCProgram() {
        let package_managers = "";

        try {
            package_managers = await invoke("run_c_program", { "function": "detect_all_package_managers" });
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

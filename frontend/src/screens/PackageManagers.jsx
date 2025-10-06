import React, { useState, useEffect, useRef } from "react";
import { invoke } from "@tauri-apps/api/core";
import { runCommand } from "../lib/run-commands.js";

const PackageManagers = () => {
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

    return (
        <div>
            <p>Test</p>
        </div>
    );
};

export default PackageManagers;

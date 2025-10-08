import React, { useState, useEffect, useRef } from "react";
import { runCommand } from "../lib/run-commands.js";

const PackageManagers = () => {
    const [packageManagers, setPackageManagers] = useState("");
    const [parsedData, setParsedData] = useState(null);

    const hasRunRef = useRef(false);

    function parsePackageManagers(logs) {
        const result = {};

        if (!logs) { return result; }

        const lines = logs.split("\n");
        let currentManager = null;

        for (const line of lines) {
        // Detect package manager section
            const managerMatch = line.match(/(apt|yum|dnf|pacman|zypper|brew|choco|winget) detected:/);
            if (managerMatch) {
                currentManager = managerMatch[1];

                // Initialize manager object with version info
                const versionMatch = line.match(/: (.+)$/);
                result[currentManager] = {
                    "version": versionMatch ? versionMatch[1].trim() : null,
                    "available": true,
                    "packages": {},
                };
                continue;
            }

            // Handle "not found" cases
            const notFoundMatch = line.match(/(apt|yum|dnf|pacman|zypper|brew|choco|winget) detected: sh: .*not found/);
            if (notFoundMatch) {
                const manager = notFoundMatch[1];
                result[manager] = {
                    "available": false,
                    "error": "Command not found",
                    "packages": {},
                };
                continue;
            }

            // Parse installed packages for current manager
            if (currentManager && result[currentManager] && result[currentManager].available && line.trim() && !line.includes("WARNING:") && !line.includes("Listing...") && !line.includes("Installed packages for") && !line.match(/sh: .*not found/)) {

                // Parse package line format: "package/version,now X.Y.Z arch [flags]"
                const packageMatch = line.match(/^\s*([^\/\s]+)\/([^,\s]+)(?:,([^ ]+))?\s+([^ ]+)\s+([^ ]+)\s+(\[.+\])?/);

                if (packageMatch) {
                    const [, packageName, version, status, repo, arch, flags] = packageMatch;
                    const cleanPackageName = packageName.trim();

                    result[currentManager].packages[cleanPackageName] = {
                        "version": version.trim(),
                        "status": status ? status.trim() : "",
                        "repository": repo.trim(),
                        "architecture": arch.trim(),
                        "flags": flags ? flags.replace(/[\[\]]/g, "").split(",").map((f) => { return f.trim(); }) : [],
                    };
                } else if (line.trim()) {
                // Fallback for packages that don't match the exact format
                    const rawLine = line.trim();
                    const fallbackName = rawLine.split("/")[0]?.trim() || rawLine;
                    result[currentManager].packages[fallbackName] = {
                        "raw": rawLine,
                        "version": "unknown",
                        "status": "unknown",
                        "repository": "unknown",
                        "architecture": "unknown",
                        "flags": [],
                    };
                }
            }
        }

        return result;
    }

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
                        const parsed = parsePackageManagers(result.value);
                        setParsedData(parsed);
                        console.log("Parsed Package Managers:", parsed);
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

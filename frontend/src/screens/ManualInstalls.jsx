import React, { useState, useEffect, useRef } from "react";
import { runCommand } from "../lib/run-commands.js";

const ManualInstalls = () => {
    const [manualInstalls, setManualInstalls] = useState("");
    const [directoryStructure, setDirectoryStructure] = useState({ "opt": { "_type": "directory", "_files": [], "_subdirs": {} } });
    const [currentPath, setCurrentPath] = useState(["opt"]);
    const [availableOptions, setAvailableOptions] = useState([]);
    const [currentFiles, setCurrentFiles] = useState([]);
    const hasRunRef = useRef(false);

    useEffect(() => {
        if (!hasRunRef.current) {
            hasRunRef.current = true;

            Promise.allSettled([
                runCommand("list_manual_installs", []).then((output) => {
                    setManualInstalls(output);
                    return output;
                }),
            ]).then((results) => {
                results.forEach((result, index) => {
                    if (result.status === "fulfilled") {
                        console.log("Raw output:", result.value);
                    }
                    if (result.status === "rejected") {
                        console.error(`Command ${index} failed: ${result.reason}`);
                    }
                });
            });
        }
    }, []);

    // Build directory structure from the output - SAFE VERSION
    useEffect(() => {
        if (!manualInstalls) { return; }

        const lines = manualInstalls.split("\n");
        const structure = {
            "opt": {
                "_type": "directory",
                "_files": [],
                "_subdirs": {},
            },
        };

        console.log("Processing lines:", lines);

        const ensureDirectory = (pathParts) => {
            let current = structure;
            
            for (let i = 0; i < pathParts.length; i++) {
                const part = pathParts[i];
                
                if (!current[part]) {
                    current[part] = {
                        "_type": "directory",
                        "_files": [],
                        "_subdirs": {},
                    };
                }
                
                // Move to subdirs for next iteration, except for the last part
                if (i < pathParts.length - 1) {
                    if (!current[part]._subdirs) {
                        current[part]._subdirs = {};
                    }
                    current = current[part]._subdirs;
                }
            }
        };

        const addFile = (filePath) => {
            const pathParts = filePath.split("/").filter((part) => { return part.length > 0; });
            
            if (pathParts[0] !== "opt" || pathParts.length < 2) { return; }
            
            // Ensure parent directory exists
            const parentPath = pathParts.slice(0, -1);
            ensureDirectory(parentPath);
            
            // Add file to parent directory
            let parent = structure;
            for (let i = 0; i < parentPath.length; i++) {
                parent = parent[parentPath[i]];
                if (i < parentPath.length - 1) {
                    parent = parent._subdirs;
                }
            }
            
            const fileName = pathParts[pathParts.length - 1];
            if (!parent._files.includes(fileName)) {
                parent._files.push(fileName);
            }
        };

        const addDirectory = (dirPath) => {
            const pathParts = dirPath.split("/").filter((part) => { return part.length > 0; });
            
            if (pathParts[0] === "opt") {
                ensureDirectory(pathParts);
            }
        };

        lines.forEach((line) => {
            const cleanLine = line.startsWith("$") ? line.substring(1) : line;
            
            // Skip empty lines and the description line
            if (!cleanLine.trim() || cleanLine.includes("Manually installed programs")) {
                return;
            }

            console.log("Processing line:", cleanLine);

            if (cleanLine.startsWith("[DIR]")) {
                // Handle directory
                const fullPath = cleanLine.substring(6).trim();
                addDirectory(fullPath);
            } else if (cleanLine.trim() && !cleanLine.startsWith("Manually installed programs")) {
                // Handle file
                const fullPath = cleanLine.trim();
                
                if (fullPath.startsWith("/opt/")) {
                    addFile(fullPath);
                }
            }
        });

        console.log("Final directory structure:", JSON.stringify(structure, null, 2));
        setDirectoryStructure(structure);
    }, [manualInstalls]);

    // Update available options and files when current path or structure changes
    useEffect(() => {
        const getCurrentDirectoryInfo = () => {
            try {
                // Start from the root structure
                let current = directoryStructure;
                let subdirs = {};
                let files = [];

                // Navigate through the current path
                for (let i = 0; i < currentPath.length; i++) {
                    const part = currentPath[i];
                    
                    if (current && current[part]) {
                        // If this is the last part of the path, get its files and subdirs
                        if (i === currentPath.length - 1) {
                            files = current[part]._files || [];
                            subdirs = current[part]._subdirs || {};
                        } else {
                            // Move to subdirs for next part
                            current = current[part]._subdirs;
                        }
                    } else {
                        // Path not found
                        return { "subdirs": [], "files": [] };
                    }
                }

                const availableDirs = Object.keys(subdirs)
                    .filter((key) => { return subdirs[key]._type === "directory"; })
                    .sort();

                return { "subdirs": availableDirs, files };
            } catch (error) {
                console.error("Error getting directory info:", error);
                return { "subdirs": [], "files": [] };
            }
        };

        const { subdirs, files } = getCurrentDirectoryInfo();
        setAvailableOptions(subdirs);
        setCurrentFiles(files);

        console.log("Current path:", currentPath);
        console.log("Available subdirs:", subdirs);
        console.log("Files:", files);
    }, [currentPath, directoryStructure]);

    const handleDirectorySelect = (directory) => {
        if (directory) {
            setCurrentPath((prev) => { return [...prev, directory]; });
        }
    };

    const handleBreadcrumbClick = (index) => {
        setCurrentPath((prev) => { return prev.slice(0, index + 1); });
    };

    const getDisplayPath = () => {
        return `/${currentPath.join("/")}`;
    };

    return (
        <div>
            <h2>Manual Installs</h2>

            {/* Breadcrumb Navigation */}
            <div>
                <span>Location: </span>
                <div>
                    {currentPath.map((part, index) => { return (
                        <React.Fragment key = {index}>
                            {index > 0 && <span>/</span>}
                            <button onClick = {() => { return handleBreadcrumbClick(index); }}>{part}</button>
                        </React.Fragment>
                    ); })}
                </div>
            </div>

            {/* Directory Selection */}
            {availableOptions.length > 0 && (
                <div>
                    <label htmlFor = "directory-select">Select Directory:</label>
                    <select id = "directory-select" value = "" onChange = {(e) => { return handleDirectorySelect(e.target.value); }}>
                        <option value = "">Choose a directory...</option>
                        {availableOptions.map((dir) => { return (
                            <option key = {dir} value = {dir}>📁 {dir}</option>
                        ); })}
                    </select>
                    <span>({availableOptions.length} directories available)</span>
                </div>
            )}

            {/* Files Display */}
            <div>
                <div>
                    <h3>Contents of {getDisplayPath()}</h3>
                    <span>({currentFiles.length} files)</span>
                </div>
                
                {currentFiles.length > 0 ? (
                    <div>
                        {currentFiles.map((file, index) => { return (
                            <div key = {index}>
                                <span>📄</span>
                                <span>{file}</span>
                            </div>
                        ); })}
                    </div>
                ) : availableOptions.length > 0 ? (
                    <p>No files in this directory. Select a subdirectory to explore further.</p>
                ) : currentPath.length > 1 ? (
                    <p>No files or subdirectories in this location.</p>
                ) : (
                    <p>Loading directory structure...</p>
                )}
            </div>
        </div>
    );
};

export default ManualInstalls;

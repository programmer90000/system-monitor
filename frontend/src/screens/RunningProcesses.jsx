import React, { useState, useEffect, useRef } from "react";
import { invoke } from "@tauri-apps/api/core";

const RunningProcesses = () => {
    const [systemInfo, setSystemInfo] = useState({
        "runningProcesses": "",
    });

    const hasRunRef = useRef(false);

    async function runCProgram() {
        const running_processes = await invoke("run_c_program", { "function": "display_running_processes" });

        setSystemInfo({
            "runningProcesses": running_processes,
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
            systemInfo.runningProcesses,
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

export default RunningProcesses;

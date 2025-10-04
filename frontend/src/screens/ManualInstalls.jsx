import React, { useState, useEffect, useRef } from "react";
import { invoke } from "@tauri-apps/api/core";

const ManualInstalls = () => {
    const [systemInfo, setSystemInfo] = useState({
        "manualInstalls": "",
    });

    const hasRunRef = useRef(false);

    async function runCProgram() {
        const manual_installs = await invoke("run_c_program", { "function": "list_manual_installs" });

        setSystemInfo({
            "manualInstalls": manual_installs,
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
            systemInfo.manualInstalls,
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

export default ManualInstalls;

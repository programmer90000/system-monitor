import React, { useState, useEffect, useRef } from "react";
import { runCommand } from "../lib/run-commands.js";

const RunningProcesses = () => {
    const [runningProcesses, setRunningProcesses] = useState("");

    const hasRunRef = useRef(false);

    useEffect(() => {
        if (!hasRunRef.current) {
            hasRunRef.current = true;

            Promise.allSettled([
                runCommand("display_running_processes", []).then((output) => {
                    setRunningProcesses(output);
                    return { "type": "runningProcesses", "value": output };
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

export default RunningProcesses;

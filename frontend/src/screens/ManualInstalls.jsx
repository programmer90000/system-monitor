import React, { useState, useEffect, useRef } from "react";
import { runCommand } from "../lib/run-commands.js";

const ManualInstalls = () => {
    const [manualInstalls, setManualInstalls] = useState("");

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

export default ManualInstalls;

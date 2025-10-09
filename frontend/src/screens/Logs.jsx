import React, { useState, useEffect, useRef } from "react";
import { runCommand } from "../lib/run-commands.js";

const Logs = () => {
    const [systemLogs, setSystemLogs] = useState();

    const hasRunRef = useRef(false);

    useEffect(() => {
        if (!hasRunRef.current) {
            hasRunRef.current = true;

            Promise.allSettled([
                runCommand("view_system_logs", []).then((output) => {
                    setSystemLogs(output);
                    return { "type": "systemLogs", "value": output };
                }),

            ]).then((results) => {

                results.forEach((result, index) => {
                    if (result.status === "fulfilled") {
                        console.log(result);
                    }
                    if (result.status === "rejected") {
                        console.error(`Command ${index} failed:`, result.reason);
                    }
                });
            });
        }
    }, []);

    return (
        <div/>
    );
};

export default Logs;

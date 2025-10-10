import React, { useState, useEffect, useRef } from "react";
import { getRunningProcesses } from "../../services/SystemService.js";

const RunningProcesses = () => {
    const [runningProcesses, setRunningProcesses] = useState("");
    const [parsedData, setParsedData] = useState(null);

    const hasRunRef = useRef(false);

    useEffect(() => {
        if (!hasRunRef.current) {
            hasRunRef.current = true;
            getRunningProcesses(setRunningProcesses, setParsedData);
        }
    }, []);

    return (
        <div>
            <p>Test</p>
        </div>
    );
};

export default RunningProcesses;

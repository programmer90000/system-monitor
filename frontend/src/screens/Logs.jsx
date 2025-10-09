import React, { useState, useEffect, useRef } from "react";
import { getLogs } from "../../services/SystemService.js";

const Logs = () => {
    const [systemLogs, setSystemLogs] = useState();
    const [journalLogs, setJournalLogs] = useState();

    const hasRunRef = useRef(false);

    useEffect(() => {
        if (!hasRunRef.current) {
            hasRunRef.current = true;
            getLogs(setSystemLogs, setJournalLogs);
        }
    }, []);

    return (
        <div/>
    );
};

export default Logs;

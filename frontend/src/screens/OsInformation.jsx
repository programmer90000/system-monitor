import React, { useState, useEffect, useRef } from "react";
import { getOsInformation } from "../../services/SystemService.js";

const OsInformation = () => {
    const [osInformation, setOsInformation] = useState({
        "osInfo": "",
        "distributionInfo": "",
        "kernelDetails": "",
        "libraryVersions": "",
        "securityInfo": "",
        "systemLimits": "",
        "unameInfo": "",
    });
    const [parsedData, setParsedData] = useState(null);

    const hasRunRef = useRef(false);

    
    useEffect(() => {
        if (!hasRunRef.current) {
            hasRunRef.current = true;

            getOsInformation(setOsInformation, setParsedData);
        }
    }, []);

    return (
        <div>
            <p>Test</p>
        </div>
    );
};

export default OsInformation;

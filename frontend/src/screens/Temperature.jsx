import React, { useState, useEffect, useRef } from "react";
import { getTemperatureInfo } from "../../services/SystemService.js";

const Temperature = () => {
    const [temperatures, setTemperatures] = useState({
        "cpuTemperature": "",
        "gpuTemperature": "",
        "vrmTemperature": "",
        "chipsetTemperature": "",
        "motherboardTemperature": "",
        "psuTemperature": "",
        "caseTemperature": "",
    });
    const [parsedData, setParsedData] = useState(null);

    const hasRunRef = useRef(false);

    
    useEffect(() => {
        if (!hasRunRef.current) {
            hasRunRef.current = true;

            getTemperatureInfo(setTemperatures, setParsedData);
        }
    }, []);

    return (
        <div>
            <p>Test</p>
        </div>
    );
};

export default Temperature;

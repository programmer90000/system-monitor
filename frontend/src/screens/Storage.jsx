import React, { useState, useEffect, useRef } from "react";
import { getStorageInfo } from "../../services/SystemService.js";

const Storage = () => {
    const [storageDevices, setStorageDevices] = useState([]);
    const [storageDevicesWithTemperatureReporting, setStorageDevicesWithTemperatureReporting] = useState([]);
    const [smartData, setSmartData] = useState({});
    const [parsedData, setParsedData] = useState(null);

    const hasRunRef = useRef(false);

    
    useEffect(() => {
        if (!hasRunRef.current) {
            hasRunRef.current = true;

            getStorageInfo(setStorageDevices, setStorageDevicesWithTemperatureReporting, setSmartData, setParsedData);
        }
    }, []);

    return (
        <div>
            <p>Test</p>
        </div>
    );
};

export default Storage;

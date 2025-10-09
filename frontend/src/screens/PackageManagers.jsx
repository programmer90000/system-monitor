import React, { useState, useEffect, useRef } from "react";
import { getPackageManagers } from "../../services/SystemService.js";

const PackageManagers = () => {
    const [packageManagers, setPackageManagers] = useState("");
    const [parsedData, setParsedData] = useState(null);

    const hasRunRef = useRef(false);

    
    useEffect(() => {
        if (!hasRunRef.current) {
            hasRunRef.current = true;

            getPackageManagers(setPackageManagers, setParsedData);
        }
    }, []);

    return (
        <div>
            <p>Test</p>
        </div>
    );
};

export default PackageManagers;

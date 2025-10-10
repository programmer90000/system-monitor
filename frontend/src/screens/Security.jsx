import React, { useState, useEffect, useRef } from "react";
import { getSecurityInformation } from "../../services/SystemService.js";

const Security = () => {
    const [securityInformation, setSecurityInformation] = useState({
        "firewallStatus": "",
        "loggedInUsers": "",
        "startupDirectories": "",
        "systemdUserServices": "",
    });

    const [parsedData, setParsedData] = useState(null);
    const hasRunRef = useRef(false);

    useEffect(() => {
        if (!hasRunRef.current) {
            hasRunRef.current = true;
            getSecurityInformation(setSecurityInformation, setParsedData);
        }
    }, []);

    return (
        <div>
            <p>Test</p>
        </div>
    );
};

export default Security;

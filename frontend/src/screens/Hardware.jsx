import React, { useEffect, useRef } from "react";
import { getHardwareData } from "../../services/SystemService.js";

const Hardware = () => {
    const hasRunRef = useRef(false);

    useEffect(() => {
        if (!hasRunRef.current) {
            hasRunRef.current = true;
            getHardwareData();
        }
    }, []);

    return (
        <div/>
    );
};

export default Hardware;

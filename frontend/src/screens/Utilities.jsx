import React, { useState } from "react";
import { scanDirectory } from "../../services/SystemService.js";

const Utilities = () => {
    const [directoryPath, setDirectoryPath] = useState("");
    const [isScanning, setIsScanning] = useState(false);

    const handleScanDirectory = () => {
        if (!directoryPath.trim()) {
            alert("Please enter a directory path");
            return;
        }

        setIsScanning(true);

        scanDirectory(directoryPath)
            .then(({ parsed }) => {
                console.log(parsed);
            })
            .catch((error) => {
                console.error("Scan failed:", error);
                alert(`Scan failed: ${error.message || error}`);
            })
            .finally(() => {
                setIsScanning(false);
            });
    };

    const handleKeyPress = (e) => {
        if (e.key === "Enter") {
            handleScanDirectory();
        }
    };

    return (
        <div>
            <div style = {{ "margin": "10px" }}>
                <input type = "text" value = {directoryPath} onChange = {(e) => { return setDirectoryPath(e.target.value); }} onKeyPress = {handleKeyPress} placeholder = "Enter directory path to scan" style = {{ "padding": "10px", "marginRight": "10px", "width": "300px", "border": "1px solid #ccc", "borderRadius": "5px", "fontSize": "14px" }} disabled = {isScanning}/>
                <button onClick = {handleScanDirectory} disabled = {isScanning} style = {{ "padding": "10px 20px", "backgroundColor": isScanning ? "#cccccc" : "#007acc", "color": "white", "border": "none", "borderRadius": "5px", "cursor": isScanning ? "not-allowed" : "pointer", "fontSize": "14px" }}>{isScanning ? "Scanning..." : "Scan Directory"}</button>
            </div>
        </div>
    );
};

export default Utilities;

import React from "react";

const Hardware = () => {
    return (
        <div className = "content-section">
            <h2>Converter</h2>
            <p>Convert between different formats.</p>
            <div className = "converter-options">
                <select>
                    <option>Length</option>
                    <option>Weight</option>
                    <option>Temperature</option>
                </select>
                <input type = "number" placeholder = "Enter value"/>
                <button>Convert</button>
            </div>
        </div>
    );
};

export default Hardware;

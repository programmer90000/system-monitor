import React from "react";

const security = () => {
    return (
        <div className = "content-section">
            <h2>Settings</h2>
            <p>Configure application preferences.</p>
            <div className = "settings-options">
                <label><input type = "checkbox"/>Dark Mode</label>
                <label><input type = "checkbox"/>Notifications</label>
                <label><input type = "checkbox"/>Auto-save</label>
            </div>
        </div>
    );
};

export default security;

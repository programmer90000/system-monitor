import React from "react";

const PackageManagers = () => {
    return (
        <div className = "content-section">
            <h2>Profile</h2>
            <p>Manage your personal profile.</p>
            <div className = "profile-info">
                <div className = "info-item">
                    <label>Name:</label>
                    <span>John Doe</span>
                </div>
                <div className = "info-item">
                    <label>Email:</label>
                    <span>john@example.com</span>
                </div>
            </div>
        </div>
    );
};

export default PackageManagers;

import React from "react";

const dashboard = () => {
    return (
        <div className = "content-section">
            <h2>Dashboard</h2>
            <p>Welcome to your dashboard overview!</p>
            <div className = "dashboard-stats">
                <div className = "stat-card">
                    <h3>Total Users</h3>
                    <p>1,234</p>
                </div>
                <div className = "stat-card">
                    <h3>Revenue</h3>
                    <p>$12,345</p>
                </div>
            </div>
        </div>
    );
};

export default dashboard;

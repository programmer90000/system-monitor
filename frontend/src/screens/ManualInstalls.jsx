import React from "react";

const ManualInstalls = () => {
    return (
        <div className = "content-section">
            <h2>Reports</h2>
            <p>View and generate detailed reports.</p>
            <div className = "reports-list">
                <button className = "report-button">Sales Report</button>
                <button className = "report-button">User Activity</button>
                <button className = "report-button">System Performance</button>
            </div>
        </div>
    );
};

export default ManualInstalls;

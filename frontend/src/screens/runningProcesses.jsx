import React from "react";

const runningProcesses = () => {
    return (
        <div className = "content-section">
            <h2>Messages</h2>
            <p>Check your inbox and sent messages.</p>
            <div className = "messages-list">
                <div className = "message">New message from Alice</div>
                <div className = "message">Project update from Bob</div>
                <div className = "message">System notification</div>
            </div>
        </div>
    );
};

export default runningProcesses;

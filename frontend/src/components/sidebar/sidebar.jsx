import React from "react";

const Sidebar = ({ 
    activeSection, 
    onSectionChange, 
    isSidebarCollapsed,
    onToggleSidebar, 
}) => {
    const sidebarItems = [
        { "id": "dashboard", "label": "Dashboard", "icon": "📊" },
        { "id": "osInformation", "label": "OS Information", "icon": "🐧" },
        { "id": "hardware", "label": "Hardware", "icon": "⚙️" },
        { "id": "temperature", "label": "Temperature", "icon": "🌡️" },
        { "id": "storage", "label": "Storage", "icon": "💾" },
        { "id": "logs", "label": "Logs", "icon": "📋" },
        { "id": "runningProcesses", "label": "Running Processes", "icon": "📈" },
        { "id": "packageManagers", "label": "Package Managers", "icon": "🛒" },
        { "id": "manualInstalls", "label": "Manual Installs", "icon": "📥" },
        { "id": "security", "label": "Security", "icon": "🛡️" },
        { "id": "utilities", "label": "Utilities", "icon": "🧰" },
    ];

    return (
        <div className = {`sidebar ${isSidebarCollapsed ? "collapsed" : ""}`}>
            <div className = "sidebar-header">
                {!isSidebarCollapsed && (<h1>System Monitor</h1>)}
                <button className = "sidebar-toggle" onClick = {onToggleSidebar}>{isSidebarCollapsed ? "➡️" : "⬅️"}</button>
            </div>

            <nav className = "sidebar-nav">
                {sidebarItems.map((item) => { return (
                    <button key = {item.id} className = {`sidebar-button ${activeSection === item.id ? "active" : ""}`} onClick = {() => { return onSectionChange(item.id); }} title = {isSidebarCollapsed ? item.label : ""}>
                        <span className = "button-icon">{item.icon}</span>
                        {!isSidebarCollapsed && (<span className = "button-label">{item.label}</span>)}
                    </button>
                ); })}
            </nav>
        </div>
    );
};

export default Sidebar;

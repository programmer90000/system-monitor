import React from "react";

const Sidebar = ({ 
    activeSection, 
    onSectionChange, 
    expandedGroups, 
    onToggleGroup, 
    isSidebarCollapsed,
    onToggleSidebar, 
}) => {
    const sidebarGroups = [
        {
            "id": "main",
            "label": "Main Navigation",
            "icon": "🏠",
            "buttons": [
                { "id": "dashboard", "label": "Dashboard", "icon": "📊" },
                { "id": "overview", "label": "Overview", "icon": "👁️" },
                { "id": "reports", "label": "Reports", "icon": "📋" },
            ],
        },
        {
            "id": "tools",
            "label": "Tools & Utilities",
            "icon": "🛠️",
            "buttons": [
                { "id": "files", "label": "File Manager", "icon": "📁" },
                { "id": "editor", "label": "Editor", "icon": "✏️" },
                { "id": "calculator", "label": "Calculator", "icon": "🧮" },
                { "id": "converter", "label": "Converter", "icon": "🔄" },
            ],
        },
        {
            "id": "account",
            "label": "Account",
            "icon": "👤",
            "buttons": [
                { "id": "profile", "label": "Profile", "icon": "👤" },
                { "id": "settings", "label": "Settings", "icon": "⚙️" },
                { "id": "messages", "label": "Messages", "icon": "💬" },
                { "id": "notifications", "label": "Notifications", "icon": "🔔" },
            ],
        },
    ];

    return (
        <div className = {`sidebar ${isSidebarCollapsed ? "collapsed" : ""}`}>
            <div className = "sidebar-header">
                {!isSidebarCollapsed && (
                    <>
                        <h1>My App</h1>
                        <p className = "sidebar-subtitle">Navigation</p>
                    </>
                )}
                <button className = "sidebar-toggle" onClick = {onToggleSidebar}>
                    {isSidebarCollapsed ? "➡️" : "⬅️"}
                </button>
            </div>
      
            <nav className = "sidebar-nav">
                {sidebarGroups.map((group) => { return (
                    <div key = {group.id} className = "sidebar-group">
                        {/* Group Header */}
                        <button className = "sidebar-group-header"
                            onClick = {() => { return onToggleGroup(group.id); }}
                            title = {isSidebarCollapsed ? group.label : ""}
                        >
                            <span className = "group-icon">{group.icon}</span>
                            {!isSidebarCollapsed && (
                                <>
                                    <span className = "group-label">{group.label}</span>
                                    <span className = {`group-chevron ${expandedGroups[group.id] ? "expanded" : ""}`}>
                                        ▼
                                    </span>
                                </>
                            )}
                        </button>

                        {/* Group Buttons */}
                        {(!isSidebarCollapsed || expandedGroups[group.id]) && (
                            <div className = {`sidebar-group-buttons ${expandedGroups[group.id] ? "expanded" : ""}`}>
                                {group.buttons.map((button) => { return (
                                    <button key = {button.id}
                                        className = {`sidebar-button ${activeSection === button.id ? "active" : ""}`}
                                        onClick = {() => { return onSectionChange(button.id); }}
                                        title = {isSidebarCollapsed ? button.label : ""}
                                    >
                                        <span className = "button-icon">{button.icon}</span>
                                        {!isSidebarCollapsed && (
                                            <span className = "button-label">{button.label}</span>
                                        )}
                                    </button>
                                ); })}
                            </div>
                        )}
                    </div>
                ); })}
            </nav>
        </div>
    );
};

export default Sidebar;

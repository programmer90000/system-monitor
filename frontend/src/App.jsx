import React, { useState } from "react";
import "./App.css";

function App() {
    const [activeSection, setActiveSection] = useState("dashboard");
    const [expandedGroups, setExpandedGroups] = useState({
        "main": true,
        "tools": true,
        "account": true,
    });
    const [isSidebarCollapsed, setIsSidebarCollapsed] = useState(false);

    const sidebarGroups = [
        {
            "id": "main",
            "label": "Main Navigation",
            "icon": "🏠",
            "buttons": [
                { "id": "dashboard", "label": "Dashboard", "icon": "📊" },
                { "id": "overview", "label": "Overview", "icon": "📕" },
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

    const handleButtonClick = (buttonId) => {
        setActiveSection(buttonId);
        console.log(`Navigating to: ${buttonId}`);
    };

    const toggleGroup = (groupId) => {
        setExpandedGroups((prev) => { return {
            ...prev,
            [groupId]: !prev[groupId],
        }; });
    };

    const toggleSidebar = () => {
        setIsSidebarCollapsed(!isSidebarCollapsed);
    };

    const renderContent = () => {
        switch (activeSection) {
        case "dashboard":
            return <div className = "content-section"><h2>Dashboard</h2><p>Welcome to your dashboard overview!</p></div>;
        case "overview":
            return <div className = "content-section"><h2>Overview</h2><p>Get a quick overview of your data.</p></div>;
        case "reports":
            return <div className = "content-section"><h2>Reports</h2><p>View and generate detailed reports.</p></div>;
        case "files":
            return <div className = "content-section"><h2>File Manager</h2><p>Manage your files and documents.</p></div>;
        case "editor":
            return <div className = "content-section"><h2>Editor</h2><p>Create and edit your content.</p></div>;
        case "calculator":
            return <div className = "content-section"><h2>Calculator</h2><p>Perform calculations and analysis.</p></div>;
        case "converter":
            return <div className = "content-section"><h2>Converter</h2><p>Convert between different formats.</p></div>;
        case "profile":
            return <div className = "content-section"><h2>Profile</h2><p>Manage your personal profile.</p></div>;
        case "settings":
            return <div className = "content-section"><h2>Settings</h2><p>Configure application preferences.</p></div>;
        case "messages":
            return <div className = "content-section"><h2>Messages</h2><p>Check your inbox and sent messages.</p></div>;
        case "notifications":
            return <div className = "content-section"><h2>Notifications</h2><p>View your notification history.</p></div>;
        default:
            return <div className = "content-section"><h2>Welcome</h2><p>Select a section to get started.</p></div>;
        }
    };

    const getActiveButtonLabel = () => {
        for (const group of sidebarGroups) {
            const button = group.buttons.find((btn) => { return btn.id === activeSection; });
            if (button) { return button.label; }
        }
        return "Dashboard";
    };

    return (
        <div className = "app">
            {/* Sidebar */}
            <div className = {`sidebar ${isSidebarCollapsed ? "collapsed" : ""}`}>
                <div className = "sidebar-header">
                    {!isSidebarCollapsed && (
                        <>
                            <h1>My App</h1>
                            <p className = "sidebar-subtitle">Navigation Menu</p>
                        </>
                    )}
                    <button className = "sidebar-toggle" onClick = {toggleSidebar}>
                        {isSidebarCollapsed ? "➡️" : "⬅️"}
                    </button>
                </div>
        
                <nav className = "sidebar-nav">
                    {sidebarGroups.map((group) => { return (
                        <div key = {group.id} className = "sidebar-group">
                            {/* Group Header */}
                            <button className = "sidebar-group-header"
                                onClick = {() => { return toggleGroup(group.id); }}
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
                                            onClick = {() => { return handleButtonClick(button.id); }}
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

            {/* Main Content */}
            <div className = "main-content">
                <header className = "content-header">
                    <div className = "content-header-left">
                        <button className = "mobile-sidebar-toggle" onClick = {toggleSidebar}>
                            ☰
                        </button>
                        <h2>{getActiveButtonLabel()}</h2>
                    </div>
                    <div className = "content-actions">
                        <button className = "action-button">Refresh</button>
                        <button className = "action-button">Help</button>
                    </div>
                </header>
        
                <main className = "content-main">
                    {renderContent()}
                </main>
            </div>
        </div>
    );
}

export default App;

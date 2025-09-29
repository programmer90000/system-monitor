import React, { useState } from "react";
import Sidebar from "./components/sidebar/sidebar";
import { Dashboard, Overview, Reports, FileManager, Editor, Calculator, Converter, Profile, Settings, Messages, Notifications } from "./screens";
import "./App.css";

function App() {
    const [activeSection, setActiveSection] = useState("dashboard");
    const [expandedGroups, setExpandedGroups] = useState({ "main": true, "tools": true, "account": true });
    const [isSidebarCollapsed, setIsSidebarCollapsed] = useState(false);

    const handleSectionChange = (sectionId) => {
        setActiveSection(sectionId);
    };

    const handleToggleGroup = (groupId) => {
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
            return <Dashboard/>;
        case "overview":
            return <Overview/>;
        case "reports":
            return <Reports/>;
        case "files":
            return <FileManager/>;
        case "editor":
            return <Editor/>;
        case "calculator":
            return <Calculator/>;
        case "converter":
            return <Converter/>;
        case "profile":
            return <Profile/>;
        case "settings":
            return <Settings/>;
        case "messages":
            return <Messages/>;
        case "notifications":
            return <Notifications/>;
        default:
            return <Dashboard/>;
        }
    };

    const getActiveButtonLabel = () => {
        const allButtons = [
            { "id": "dashboard", "label": "Dashboard" },
            { "id": "overview", "label": "Overview" },
            { "id": "reports", "label": "Reports" },
            { "id": "files", "label": "File Manager" },
            { "id": "editor", "label": "Editor" },
            { "id": "calculator", "label": "Calculator" },
            { "id": "converter", "label": "Converter" },
            { "id": "profile", "label": "Profile" },
            { "id": "settings", "label": "Settings" },
            { "id": "messages", "label": "Messages" },
            { "id": "notifications", "label": "Notifications" },
        ];

        const button = allButtons.find((btn) => { return btn.id === activeSection; });
        return button ? button.label : "Dashboard";
    };

    return (
        <div className = "app">
            <Sidebar activeSection = {activeSection} onSectionChange = {handleSectionChange} expandedGroups = {expandedGroups} onToggleGroup = {handleToggleGroup} isSidebarCollapsed = {isSidebarCollapsed} onToggleSidebar = {toggleSidebar}/>
            <div className = "main-content">
                <header className = "content-header">
                    <div className = "content-header-left">
                        <button className = "mobile-sidebar-toggle" onClick = {toggleSidebar}/>
                        <h2>{getActiveButtonLabel()}</h2>
                    </div>
                </header>
                <main className = "content-main">{renderContent()}</main>
            </div>
        </div>
    );
}

export default App;

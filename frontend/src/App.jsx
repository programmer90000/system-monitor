import React, { useState, useEffect, useRef } from "react";
import { invoke } from "@tauri-apps/api/core";
import Sidebar from "./components/sidebar/sidebar";
import { dashboard, osInformation, hardware, temperature, storage, logs, runningProcesses, packageManagers, manualInstalls, security, utilities } from "./screens";
import "./app.css";

function App() {
    const [cProgramOutput, setCProgramOutput] = useState("");
    const [activeSection, setActiveSection] = useState("dashboard");
    const [expandedGroups, setExpandedGroups] = useState({ "main": true, "tools": true, "account": true });
    const [isSidebarCollapsed, setIsSidebarCollapsed] = useState(false);

    const hasRunRef = useRef(false);

    async function runCProgram() {
        const output = await invoke("run_c_program", { "function": "calculate_cpu_usage" });
        setCProgramOutput(output);
        console.log(cProgramOutput);
    }

    useEffect(() => {
        if (!hasRunRef.current) {
            hasRunRef.current = true;
            runCProgram();
        }
    }, []);

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
            return <dashboard/>;
        case "osInformation":
            return <osInformation/>;
        case "hardware":
            return <hardware/>;
        case "temperature":
            return <temperature/>;
        case "storage":
            return <storage/>;
        case "logs":
            return <logs/>;
        case "runningProcesses":
            return <runningProcesses/>;
        case "packageManagers":
            return <packageManagers/>;
        case "manualInstalls":
            return <manualInstalls/>;
        case "security":
            return <security/>;
        case "utilities":
            return <utilities/>;
        default:
            return <dashboard/>;
        }
    };

    const getActiveButtonLabel = () => {
        const allButtons = [
            { "id": "dashboard", "label": "Dashboard" },
            { "id": "osInformation", "label": "OS Information" },
            { "id": "hardware", "label": "Hardware" },
            { "id": "temperature", "label": "Temperature" },
            { "id": "storage", "label": "Storage" },
            { "id": "logs", "label": "Logs" },
            { "id": "runningProcesses", "label": "Running Processes" },
            { "id": "packageManagers", "label": "Package Managers" },
            { "id": "manualInstalls", "label": "Manual Installs" },
            { "id": "security", "label": "Security" },
            { "id": "utilities", "label": "Utilities" },
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

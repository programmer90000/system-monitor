import React, { useState, useEffect, useRef } from "react";
import { invoke } from "@tauri-apps/api/core";
import Sidebar from "./components/Sidebar/Sidebar";
import { Dashboard, OsInformation, Hardware, Temperature, Storage, Logs, RunningProcesses, PackageManagers, ManualInstalls, Security, Utilities } from "./screens";
import "./App.css";

function App() {
    const [cProgramOutput, setCProgramOutput] = useState("");
    const [sudoCommandOutput, setSudoCommandOutput] = useState("");
    const [activeSection, setActiveSection] = useState("dashboard");
    const [expandedGroups, setExpandedGroups] = useState({ "main": true, "tools": true, "account": true });
    const [isSidebarCollapsed, setIsSidebarCollapsed] = useState(false);

    const hasRunRef = useRef(false);

    async function runCProgram(functionName, args = []) {
        const output = await invoke("run_c_program", { "function": functionName, args });
        setCProgramOutput(output);
        console.log(output);
    }

    async function runSudoCommand(functionName, args = []) {
        const output = await invoke("run_sudo_command", { "function": functionName, args });
        setSudoCommandOutput(output);
        console.log("Sudo command output:", output);
    }

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
        case "osInformation":
            return <OsInformation/>;
        case "hardware":
            return <Hardware/>;
        case "temperature":
            return <Temperature/>;
        case "storage":
            return <Storage/>;
        case "logs":
            return <Logs/>;
        case "runningProcesses":
            return <RunningProcesses/>;
        case "packageManagers":
            return <PackageManagers/>;
        case "manualInstalls":
            return <ManualInstalls/>;
        case "security":
            return <Security/>;
        case "utilities":
            return <Utilities/>;
        default:
            return <Dashboard/>;
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

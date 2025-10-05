import React, { useState, useEffect, useRef } from "react";
import { invoke } from "@tauri-apps/api/core";

const Security = () => {
    const [systemInfo, setSystemInfo] = useState({
        "firewallStatus": "",
        "loggedInUsers": "",
        "startupDirectories": "",
        "systemdUserServices": "",
    });

    const hasRunRef = useRef(false);

    async function runCProgram() {
        let firewall_status = "";
        let logged_in_users = "";
        let startup_directories = "";
        let systemd_user_services = "";

        try {
            firewall_status = await invoke("run_c_program", { "function": "check_firewall" });
            console.log("Firewall Status - Data Loaded");
        } catch (error) {
            console.error("Error fetching firewall status:", error);
            firewall_status = "Error: Failed to fetch firewall status";
        }

        try {
            logged_in_users = await invoke("run_c_program", { "function": "show_logged_in_users" });
            console.log("Logged In Users - Data Loaded");
        } catch (error) {
            console.error("Error fetching logged in users:", error);
            logged_in_users = "Error: Failed to fetch logged in users";
        }

        try {
            startup_directories = await invoke("run_c_program", { "function": "check_startup_directories" });
            console.log("Startup Directories - Data Loaded");
        } catch (error) {
            console.error("Error fetching startup directories:", error);
            startup_directories = "Error: Failed to fetch startup directories";
        }

        try {
            systemd_user_services = await invoke("run_c_program", { "function": "check_systemd_user_services" });
            console.log("Systemd User Services - Data Loaded");
        } catch (error) {
            console.error("Error fetching systemd user services:", error);
            systemd_user_services = "Error: Failed to fetch systemd user services";
        }

        setSystemInfo({
            "firewallStatus": firewall_status,
            "loggedInUsers": logged_in_users,
            "startupDirectories": startup_directories,
            "systemdUserServices": systemd_user_services,
        });
    }

    useEffect(() => {
        if (!hasRunRef.current) {
            hasRunRef.current = true;
            runCProgram();
        }
    }, []);

    useEffect(() => {
        if (!systemInfo) { return; }

        // All system info blocks
        const allBlocks = [
            systemInfo.firewallStatus,
            systemInfo.loggedInUsers,
            systemInfo.startupDirectories,
            systemInfo.systemdUserServices,
        ];

        // Keep allValues the same (values only)
        const allValues = allBlocks.flatMap((block) =>
        { return (block || "")
            .split("\n")
            .map((line) => { return line.trim(); })
            .map((line) => {
                if (!line || (/^={3,}/).test(line) || (/^-{3,}/).test(line)) { return null; }
                const match = line.match(/^[^=:]+[=:]\s*(.*)$/);
                return match ? match[1].replace(/^"+|"+$/g, "").trim() : line;
            })
            .filter(Boolean); },
        );

        // Log with simple sequential keys
        allValues.forEach((value) => {
            console.log(value);
        });

    }, [systemInfo]);

    return (
        <div>
            <p>Test</p>
        </div>
    );
};

export default Security;

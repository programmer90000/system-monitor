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
        const firewall_status = await invoke("run_c_program", { "function": "check_firewall" });
        const logged_in_users = await invoke("run_c_program", { "function": "show_logged_in_users" });
        const startup_directories = await invoke("run_c_program", { "function": "check_startup_directories" });
        const systemd_user_services = await invoke("run_c_program", { "function": "check_systemd_user_services" });

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

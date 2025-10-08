import React, { useState, useEffect, useRef } from "react";
import { runCommand } from "../lib/run-commands.js";

const Security = () => {
    const [firewallStatus, setFirewallStatus] = useState("");
    const [loggedInUsers, setLoggedInUsers] = useState("");
    const [startupDirectories, setStartupDirectories] = useState("");
    const [systemdUserServices, setSystemdUserServices] = useState("");
    const [parsedData, setParsedData] = useState(null);

    const hasRunRef = useRef(false);

    function parseSecurityInfo(logs) {
        const result = {};

        if (logs.firewallStatus) {
            const firewallText = logs.firewallStatus.replace(/^"|\s*"$/g, "").trim();
            result.firewallStatus = {
                "iptables": firewallText.match(/iptables:\s*([^\n]+)/)?.[1]?.trim(),
                "nftables": firewallText.match(/nftables:\s*([^\n]+)/)?.[1]?.trim(),
                "ufw": firewallText.match(/UFW:\s*([^\n]+)/)?.[1]?.trim(),
                "overallStatus": firewallText.match(/Overall firewall status:\s*([^\n]+)/)?.[1]?.trim(),
            };
        }

        if (logs.loggedInUsers) {
            const usersText = logs.loggedInUsers.replace(/^"|\s*"$/g, "").trim();
            const lines = usersText.split("\n");

            result.loggedInUsers = {
                "systemInfo": lines[0] || "",
                "users": [],
            };

            // Parse user sessions (skip header line)
            for (let i = 1; i < lines.length; i++) {
                const line = lines[i].trim();
                if (line && !line.includes("USER") && !line.includes("TTY") && !line.includes("FROM")) {
                    const parts = line.split(/\s+/);
                    if (parts.length >= 7) {
                        result.loggedInUsers.users.push({
                            "user": parts[0],
                            "tty": parts[1],
                            "from": parts[2],
                            "loginTime": parts[3],
                            "idle": parts[4],
                            "jcpu": parts[5],
                            "pcpu": parts[6],
                            "command": parts.slice(7).join(" "),
                        });
                    }
                }
            }
        }

        if (logs.startupDirectories) {
            const startupText = logs.startupDirectories.replace(/^"|\s*"$/g, "").trim();
            const sections = startupText.split("Checking:");

            result.startupDirectories = {
                "userAutostart": [],
                "systemAutostart": [],
                "scriptsDirectory": "",
                "kdeAutostart": "",
                "localAutostart": "",
            };

            sections.forEach((section) => {
                if (section.includes("/home/") && section.includes("/.config/autostart")) {
                    const files = section.split("\n")
                        .filter((line) => { return line.trim() && !line.includes("Checking:") && !line.includes("Directory"); })
                        .map((line) => { return line.trim(); });
                    result.startupDirectories.userAutostart = files;
                } else if (section.includes("/etc/xdg/autostart")) {
                    const files = section.split("\n")
                        .filter((line) => { return line.trim() && !line.includes("Checking:") && !line.includes("Directory"); })
                        .map((line) => { return line.trim(); });
                    result.startupDirectories.systemAutostart = files;
                } else if (section.includes("/home/") && section.includes("/.config/autostart-scripts")) {
                    result.startupDirectories.scriptsDirectory = section.includes("Directory not found") ? "not found" : "found";
                } else if (section.includes("/home/") && section.includes("/.kde/Autostart")) {
                    result.startupDirectories.kdeAutostart = section.includes("Directory not found") ? "not found" : "found";
                } else if (section.includes("/home/") && section.includes("/.local/share/autostart")) {
                    result.startupDirectories.localAutostart = section.includes("Directory not found") ? "not found" : "found";
                }
            });
        }

        if (logs.systemdUserServices) {
            const systemdText = logs.systemdUserServices.replace(/^"|\s*"$/g, "").trim();
            const lines = systemdText.split("\n");
            
            result.systemdUserServices = {
                "services": [],
                "enabledServices": [],
            };

            lines.forEach((line) => {
                const trimmed = line.trim();
                if (trimmed && !trimmed.includes("===")) {
                    const serviceMatch = trimmed.match(/^([\w-]+\.service)\s+(\w+)\s+(\w+)/);
                    if (serviceMatch) {
                        const service = {
                            "name": serviceMatch[1],
                            "loadState": serviceMatch[2],
                            "activeState": serviceMatch[3],
                        };
                        result.systemdUserServices.services.push(service);

                        if (service.loadState === "enabled") {
                            result.systemdUserServices.enabledServices.push(service.name);
                        }
                    }
                }
            });
        }

        return result;
    }

    useEffect(() => {
        if (!hasRunRef.current) {
            hasRunRef.current = true;

            Promise.allSettled([
                runCommand("check_firewall", []).then((output) => {
                    setFirewallStatus(output);
                    return { "type": "firewallStatus", "value": output };
                }),

                runCommand("show_logged_in_users", []).then((output) => {
                    setLoggedInUsers(output);
                    return { "type": "loggedInUsers", "value": output };
                }),

                runCommand("check_startup_directories", []).then((output) => {
                    setStartupDirectories(output);
                    return { "type": "startupDirectories", "value": output };
                }),

                runCommand("check_systemd_user_services", []).then((output) => {
                    setSystemdUserServices(output);
                    return { "type": "systemdUserServices", "value": output };
                }),
                
            ]).then((results) => {
                const allData = {};
                results.forEach((result) => {
                    if (result.status === "fulfilled") {
                        allData[result.value.type] = result.value.value;
                        console.log(`${result.value.type}:`, result.value.value);
                    }
                    if (result.status === "rejected") {
                        console.error("Command failed:", result.reason);
                    }
                });

                const parsed = parseSecurityInfo(allData);
                setParsedData(parsed);
                console.log("Parsed Security Info:", parsed);
            });
        }
    }, []);

    return (
        <div>
            <p>Test</p>
        </div>
    );
};

export default Security;

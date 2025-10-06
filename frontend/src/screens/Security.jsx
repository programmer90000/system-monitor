import React, { useState, useEffect, useRef } from "react";
import { runCommand } from "../lib/run-commands.js";

const Security = () => {
    const [firewallStatus, setFirewallStatus] = useState("");
    const [loggedInUsers, setLoggedInUsers] = useState("");
    const [startupDirectories, setStartupDirectories] = useState("");
    const [systemdUserServices, setSystemdUserServices] = useState("");

    const hasRunRef = useRef(false);

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
                results.forEach((result, index) => {
                    if (result.status === "fulfilled") {
                        console.log(`${result.value.type}:`, result.value.value);
                    }
                    if (result.status === "rejected") {
                        console.error(`Command ${index} failed:`, result.reason);
                    }
                });
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

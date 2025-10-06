import React from "react";
import { invoke } from "@tauri-apps/api/core";

async function runCommand(functionName, args = []) {
    const output = await invoke("run_c_program", { "function": functionName, args });
    return output;
}

async function runSudoCommand(functionName, args = []) {
    const output = await invoke("run_sudo_command", { "function": functionName, args });
    return output;
}

export { runCommand, runSudoCommand };

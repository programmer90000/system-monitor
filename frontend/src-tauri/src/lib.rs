use tauri::command;
use std::process::Command;
use std::path::PathBuf;

// Learn more about Tauri commands at https://tauri.app/develop/calling-rust/

#[tauri::command]
fn run_c_program(function: &str) -> String {
    // Use absolute path from the project root
    let mut c_program_path = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    c_program_path.pop(); // Go up from src-tauri
    c_program_path.pop(); // Go up from frontend  
    c_program_path.push("backend");
    c_program_path.push("system-monitor");
    
    println!("Looking for C program at: {:?}", c_program_path);
    println!("Calling function: {}", function);
    
    // Execute the C program with the function argument
    match Command::new(&c_program_path).arg(function).output() {
        Ok(output) => {
            if output.status.success() {
                let stdout = String::from_utf8_lossy(&output.stdout).to_string();
                println!("C program output: {}", stdout);
                stdout
            } else {
                let stderr = String::from_utf8_lossy(&output.stderr).to_string();
                println!("C program error: {}", stderr);
                format!("Error: {}", stderr)
            }
        }
        Err(e) => {
            let error_msg = format!("Failed to execute C program at {:?}: {}", c_program_path, e);
            println!("{}", error_msg);
            error_msg
        }
    }
}

#[tauri::command]
fn run_sudo_command(function: String, args: Vec<String>) -> String {
    println!("Attempting to run sudo command: {} {:?}", function, args);
    
    // Use pkexec for GUI password prompt handled by the system
    let output = Command::new("pkexec")
        .arg(&function)
        .args(&args)
        .output();
    
    match output {
        Ok(output) => {
            if output.status.success() {
                let stdout = String::from_utf8_lossy(&output.stdout).to_string();
                println!("Sudo command executed successfully");
                stdout
            } else {
                let stderr = String::from_utf8_lossy(&output.stderr).to_string();
                println!("Sudo command failed: {}", stderr);
                
                // Compare Option<i32> with Some(126) instead of 126
                if stderr.contains("not authorized") || output.status.code() == Some(126) {
                    "Error: Authentication failed or cancelled".to_string()
                } else {
                    format!("Error: {}", stderr)
                }
            }
        }
        Err(e) => {
            let error_msg = format!("Failed to execute sudo command: {}", e);
            println!("{}", error_msg);
            
            if error_msg.contains("No such file or directory") {
                "Error: pkexec not available on this system".to_string()
            } else {
                error_msg
            }
        }
    }
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_opener::init())
        .invoke_handler(tauri::generate_handler![run_c_program, run_sudo_command])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}

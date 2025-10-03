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

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_opener::init())
        .invoke_handler(tauri::generate_handler![run_c_program])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}

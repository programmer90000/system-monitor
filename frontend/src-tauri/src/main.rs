// ! DO NOT REMOVE!!!
// Prevents additional console window on Windows in release
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

fn main() {
    system_monitor_lib::run()
}

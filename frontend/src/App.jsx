import { useState, useEffect } from "react";
import reactLogo from "./assets/react.svg";
import { invoke } from "@tauri-apps/api/core";
import "./App.css";

function App() {
    const [greetMsg, setGreetMsg] = useState("");
    const [name, setName] = useState("");
    const [cProgramOutput, setCProgramOutput] = useState("");

    async function greet() {
        // Learn more about Tauri commands at https://tauri.app/develop/calling-rust/
        setGreetMsg(await invoke("greet", { name }));
    }

    async function runCProgram() {
        const output = await invoke("run_c_program");
        setCProgramOutput(output);
    }

    useEffect(() => {
        runCProgram();
    }, []);

    return (
        <main className = "container">
            <h1>Welcome to Tauri + React</h1>

            <div className = "row">
                <a href = "https://vite.dev" target = "_blank">
                    <img src = "/vite.svg" className = "logo vite" alt = "Vite logo"/>
                </a>
                <a href = "https://tauri.app" target = "_blank">
                    <img src = "/tauri.svg" className = "logo tauri" alt = "Tauri logo"/>
                </a>
                <a href = "https://react.dev" target = "_blank">
                    <img src = {reactLogo} className = "logo react" alt = "React logo"/>
                </a>
            </div>

            <div className = "row">
                <div style = {{ "textAlign": "left", "margin": "20px", "padding": "15px", "backgroundColor": "#000000", "borderRadius": "8px" }}>
                    <h3>C Program Output</h3>
                    <pre style = {{ "whiteSpace": "pre-wrap" }}>{cProgramOutput || "Loading..."}</pre>
                    <button onClick = {runCProgram}>Run C Program Again</button>
                </div>
            </div>

            <p>Click on the Tauri, Vite, and React logos to learn more.</p>

            <form className = "row" onSubmit = {(e) => {
                e.preventDefault();
                greet();
            }}
            >
                <input id = "greet-input" onChange = {(e) => { return setName(e.currentTarget.value); }} placeholder = "Enter a name..."/>
                <button type = "submit">Greet</button>
            </form>
            <p>{greetMsg}</p>
        </main>
    );
}

export default App;

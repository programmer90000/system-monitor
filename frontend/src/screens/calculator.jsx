import React, { useState } from "react";

const Calculator = () => {
    const [input, setInput] = useState("");
    const [result, setResult] = useState("");

    const handleCalculate = () => {
        try {
            setResult(eval(input));
        } catch (error) {
            setResult("Error");
        }
    };

    return (
        <div className = "content-section">
            <h2>Calculator</h2>
            <p>Perform calculations and analysis.</p>
            <div className = "calculator">
                <input type = "text" 
                    value = {input} 
                    onChange = {(e) => { return setInput(e.target.value); }}
                    placeholder = "Enter calculation"
                />
                <button onClick = {handleCalculate}>Calculate</button>
                <div className = "result">Result: {result}</div>
            </div>
        </div>
    );
};

export default Calculator;

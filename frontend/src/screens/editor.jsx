import React from "react";

const Editor = () => {
    return (
        <div className = "content-section">
            <h2>Editor</h2>
            <p>Create and edit your content.</p>
            <textarea className = "editor-textarea" 
                placeholder = "Start typing your content here..."
                rows = "10"
            />
        </div>
    );
};

export default Editor;

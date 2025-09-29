import React from "react";

const FileManager = () => {
    return (
        <div className = "content-section">
            <h2>File Manager</h2>
            <p>Manage your files and documents.</p>
            <div className = "file-list">
                <div className = "file-item">Document.pdf</div>
                <div className = "file-item">Spreadsheet.xlsx</div>
                <div className = "file-item">Presentation.pptx</div>
            </div>
        </div>
    );
};

export default FileManager;

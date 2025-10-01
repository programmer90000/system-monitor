# How to run the app

Use the GCC compiler to compile the backend app:
```
cd backend/
gcc system-monitor.c -o system-monitor
```

Run the frontend app:
```
cd frontend/
npm install
npm run tauri dev
```

# How to install the app

Use the GCC compiler to compile the backend app:
```
cd backend/
gcc system-monitor.c -o system-monitor
```

Install the frontend app:
```
cd frontend/
npm install
npm run tauri dev
npm run tauri build -- --target x86_64-unknown-linux-gnu
```

This will make a file:
```
system-monitor/frontend/src-tauri/target/x86_64-unknown-linux-gnu/release/bundle/deb/system-monitor_0.1.0_amd64.deb
```

Open this file

This will open the Discover app

Install the System Monitor app 

Open the app by clicking Launch

To view the console output, run the app in the terminal instead:
```
system-monitor
```
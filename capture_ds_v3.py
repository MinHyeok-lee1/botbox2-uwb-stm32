import serial
import threading
import time
import subprocess
import os

ports = {
    "TAG": ("COM18", "004500403234511137333934"),
    "ANC1": ("COM19", "0031001F4142501820353451"),
    "ANC2": ("COM15", "0032002E3234510A37333934"),
    "ANC3": ("COM20", "001C00283234511137333934")
}

log_dir = "artifacts/ioc_shell_ds_smoke/20260428-1822"
cli = r"C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"

stop_event = threading.Event()

def read_port(name, port_name):
    filename = f"{name.lower()}_{port_name.lower()}.log"
    try:
        with serial.Serial(port_name, 115200, timeout=0.5) as ser:
            with open(os.path.join(log_dir, filename), "w", encoding='utf-8', errors='ignore') as f:
                while not stop_event.is_set():
                    line = ser.readline()
                    if line:
                        f.write(line.decode('utf-8', errors='ignore'))
                        f.flush()
    except Exception as e:
        print(f"Error on {name} ({port_name}): {e}")

threads = []
for name, (port, sn) in ports.items():
    t = threading.Thread(target=read_port, args=(name, port))
    t.start()
    threads.append(t)

time.sleep(2)

print("Resetting boards via SWD...")
for name, (port, sn) in ports.items():
    subprocess.run([cli, "-c", f"port=SWD", f"sn={sn}", "-rst"], capture_output=True)

print("Recording for 65 seconds...")
time.sleep(65)
stop_event.set()

for t in threads:
    t.join()

print(f"Done recording to artifacts/ioc_shell_ds_smoke/20260428-1822")

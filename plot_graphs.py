import json
import matplotlib.pyplot as plt
from datetime import datetime
import matplotlib.dates as mdates

#json
with open("esp32-ag_data.json", "r") as f:
    data = json.load(f)

times = [datetime.strptime(d["time"], "%H:%M:%S") for d in data]
wifi = [d["WiFi APs"] for d in data]
ble = [d["BLE devices"] for d in data]
rssi = [d["Target BLE RSSI"] for d in data]

start_time = datetime.strptime("08:02", "%H:%M").time()
end_time   = datetime.strptime("17:30", "%H:%M").time()
mask = [start_time <= t.time() <= end_time for t in times]

times = [t for t, m in zip(times, mask) if m]
wifi  = [v for v, m in zip(wifi, mask) if m]
ble   = [v for v, m in zip(ble, mask) if m]
rssi  = [v for v, m in zip(rssi, mask) if m]

fig, axes = plt.subplots(2, 1, figsize=(20, 10), sharex=True)

#Graph 1
ax1 = axes[0]
ax1.plot(times, wifi, color="red" , label="Number of WiFi APs")
ax1.plot(times, ble, color="green", label="Number of BLE devices")
ax1.set_ylabel("Nb of BLE devices and WiFi APs")

ax1b = ax1.twinx()
ax1b.set_title("Number of WiFi APs and BLE devices evolution through the day")
ax1b.grid(True, which="major", linestyle="--", alpha=0.3)

#Legend
lines1, labels1 = ax1.get_legend_handles_labels()
lines2, labels2 = ax1b.get_legend_handles_labels()
ax1.legend(lines1 + lines2, labels1 + labels2)

#Graph 2
ax2 = axes[1]
ax2.plot(times, rssi)
ax2.set_ylabel("RSSI (dBm)")
ax2.set_title("Target BLE RSSI evolution through the day")
ax2.grid(True, which="major", linestyle="--", alpha=0.3)

ax1.set_xlabel("Time")
ax1.xaxis.set_major_formatter(mdates.DateFormatter("%H:%M"))
plt.xticks(rotation=45)

locator = mdates.MinuteLocator(byminute=[0, 30])
formatter = mdates.DateFormatter("%H:%M")

for ax in axes:
    ax.xaxis.set_major_locator(locator)
    ax.xaxis.set_major_formatter(formatter)

#display
plt.tight_layout()
plt.savefig("graphs.png", dpi=300, bbox_inches="tight")
plt.show()


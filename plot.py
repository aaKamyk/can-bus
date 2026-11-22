import csv
import matplotlib.pyplot as plt


def plot_bus_load(csv_filepath: str = "bus_load.csv") -> None:
    time_us: list[float] = []
    bus_load: list[float] = []

    with open(csv_filepath, mode="r", encoding="utf-8") as file:
        reader = csv.DictReader(file, delimiter=';')
        for row in reader:
            time_us.append(float(row["time_us"]))
            bus_load.append(float(row["bus_load_percent"]))

    avg_load = sum(bus_load) / len(bus_load)

    plt.figure(figsize=(10, 5))
    plt.plot(time_us, bus_load, label="Bus Load (%)", color="#1f77b4", linewidth=1.5)
    plt.axhline(
        avg_load,
        color="#d62728",
        linestyle="--",
        linewidth=1.2,
        label=f"Average Load ({avg_load:.2f}%)",
    )

    plt.title("CAN Bus Load Over Time")
    plt.xlabel("Time [us]")
    plt.ylabel("Bus Load [%]")
    plt.ylim(0, max(100.0, max(bus_load) * 1.1))
    plt.grid(True, linestyle=":", alpha=0.7)
    plt.legend()
    plt.tight_layout()

    plt.savefig("bus_load.png", dpi=300)
    plt.show()


if __name__ == "__main__":
    plot_bus_load()
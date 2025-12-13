import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from pathlib import Path

# Load Friday data
friday_alighters = pd.read_csv("csv_day/FRI/NBT23FRI_filtered_Station_Alighters.csv")

# Display columns to understand structure
print(friday_alighters.columns)
print(friday_alighters.head())

# Get time period columns (exclude non-numeric columns)
time_cols = [
    col
    for col in friday_alighters.columns
    if col
    not in [
        "ID",
        "NLC",
        "ASC",
        "Station",
        "Mode",
        "Line",
        "Dir",
        "Platform",
        "Total",
        "Early     ",
        "AM Peak   ",
        "Midday    ",
        "PM Peak   ",
        "Evening   ",
        "Late      ",
    ]
]

# ===== 1. HEATMAP: Time vs Station vs Alighters =====
# Group by station and sum across all directions/platforms
heatmap_data = friday_alighters.groupby("Station")[time_cols].sum()

plt.figure(figsize=(20, 10))
sns.heatmap(heatmap_data, cmap="YlOrRd", cbar_kws={"label": "Alighters"})
plt.title("Jubilee Line Station Alighters Heatmap - Friday 2023", fontsize=16)
plt.xlabel("Time Period", fontsize=12)
plt.ylabel("Station", fontsize=12)
plt.xticks(rotation=90)
plt.tight_layout()
plt.savefig("heatmap_alighters_friday.png", dpi=300)
plt.show()

# ===== 2. TIME SERIES: Stratford over time =====
stratford_data = friday_alighters[friday_alighters["Station"] == "Stratford"]

# Sum across all platforms/directions for Stratford
stratford_series = stratford_data[time_cols].sum().values

plt.figure(figsize=(15, 6))
plt.plot(time_cols, stratford_series, marker="o", linewidth=2, markersize=4)
plt.title("Stratford Station Alighters - Friday 2023", fontsize=16)
plt.xlabel("Time Period", fontsize=12)
plt.ylabel("Number of Alighters", fontsize=12)
plt.xticks(rotation=90)
plt.grid(True, alpha=0.3)
plt.tight_layout()
plt.savefig("timeseries_stratford_friday.png", dpi=300)
plt.show()

# ===== 3. BAR PLOT: Peak morning hour (e.g., 0800-0815) =====
time_slot = "0800-0815"

if time_slot in time_cols:
    # Group by station and sum for the time slot
    station_values = (
        friday_alighters.groupby("Station")[time_slot]
        .sum()
        .sort_values(ascending=False)
    )

    plt.figure(figsize=(14, 8))
    plt.bar(range(len(station_values)), station_values.values)
    plt.xticks(range(len(station_values)), station_values.index, rotation=90)
    plt.title(f"Alighters by Station during {time_slot} - Friday 2023", fontsize=16)
    plt.xlabel("Station", fontsize=12)
    plt.ylabel("Number of Alighters", fontsize=12)
    plt.tight_layout()
    plt.savefig("barplot_timeslot_friday.png", dpi=300)
    plt.show()
else:
    print(f"Time slot {time_slot} not found. Available columns: {time_cols[:5]}...")

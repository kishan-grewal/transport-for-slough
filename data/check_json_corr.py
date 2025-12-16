"""
OD Matrix Correlation Analysis

Extracts individual OD matrices for each day and direction, then computes
pairwise correlations to validate whether OD probabilities are stable across days.

Outputs:
- JSON files in json_corr/ (MON_NB.json, MON_SB.json, etc.)
- correlation_results.txt with correlation matrices
- correlation_heatmaps.png visualising the results
"""

import pandas as pd
import json
import numpy as np
from pathlib import Path
from typing import Dict, List
import warnings

warnings.filterwarnings("ignore")

# Same configuration as get_data.py
JUBILEE_STATIONS = [
    "Stratford",
    "West Ham",
    "Canning Town",
    "North Greenwich",
    "Canary Wharf LU",
    "Canada Water",
    "Bermondsey",
    "London Bridge LU",
    "Southwark",
    "Waterloo LU",
    "Westminster",
    "Green Park",
    "Bond Street",
    "Baker Street",
    "St. John's Wood",
    "Swiss Cottage",
    "Finchley Road",
    "West Hampstead LU",
    "Kilburn",
    "Willesden Green",
    "Dollis Hill",
    "Neasden",
    "Wembley Park",
    "Kingsbury",
    "Queensbury",
    "Canons Park",
    "Stanmore",
]

STATION_NAME_MAPPING = {
    "Canary Wharf LU": "Canary Wharf",
    "London Bridge LU": "London Bridge",
    "Waterloo LU": "Waterloo",
    "West Hampstead LU": "West Hampstead",
}

BASE_DIR = Path(__file__).resolve().parent
DATA_FOLDER = BASE_DIR
INPUT_FOLDER = DATA_FOLDER / "raw_excel"
OUTPUT_FOLDER = DATA_FOLDER / "json_corr"

INPUT_FILES = [
    ("NBT23MON_outputs.xlsx", "MON"),
    ("NBT23TWT_outputs.xlsx", "TWT"),
    ("NBT23FRI_outputs.xlsx", "FRI"),
    ("NBT23SAT_outputs.xlsx", "SAT"),
    ("NBT23SUN_outputs.xlsx", "SUN"),
]


def normalize_station_name(name: str) -> str:
    return STATION_NAME_MAPPING.get(name, name)


def load_sheet(file_path: Path, sheet_name: str, header_row: int = 2) -> pd.DataFrame:
    try:
        df = pd.read_excel(file_path, sheet_name=sheet_name, header=header_row)
        print(f"  Loaded {sheet_name}: {len(df)} rows")
        return df
    except Exception as e:
        print(f"  Error loading {sheet_name}: {e}")
        return pd.DataFrame()


def build_od_matrix_from_links(link_data: pd.DataFrame) -> Dict[str, Dict[str, float]]:
    """Extract OD matrix from link loads using alighting inference."""
    od_matrix = {}

    for origin_idx in range(len(link_data)):
        origin = link_data.iloc[origin_idx]["From Station"]
        initial_load = link_data.iloc[origin_idx]["Total"]

        if initial_load <= 0:
            continue

        destinations = {}
        remaining_load = initial_load

        for dest_idx in range(origin_idx + 1, len(link_data) + 1):
            if dest_idx == len(link_data):
                terminal = link_data.iloc[-1]["To Station"]
                if remaining_load > 0:
                    destinations[terminal] = remaining_load / initial_load
                break

            load_before = link_data.iloc[dest_idx - 1]["Total"]
            load_after = link_data.iloc[dest_idx]["Total"]
            alighters = max(0, load_before - load_after)

            station = link_data.iloc[dest_idx]["From Station"]

            if alighters > 0:
                destinations[station] = alighters / initial_load
                remaining_load -= alighters

        total = sum(destinations.values())
        if total > 0:
            destinations = {k: v / total for k, v in destinations.items()}

        od_matrix[origin] = destinations

    return od_matrix


def extract_od_matrix_for_day(file_path: Path, day_code: str) -> Dict[str, Dict]:
    """Extract OD matrices for both directions from a single day's data."""
    print(f"\nProcessing {file_path.name} ({day_code})")

    df = load_sheet(file_path, "Link_Loads", header_row=2)

    if df.empty:
        return {}

    jubilee = df[df["Line"] == "Jubilee"].copy()
    od_matrices = {}

    for direction in ["NB", "SB"]:
        print(f"  Extracting {direction} OD matrix")

        dir_data = jubilee[jubilee["Dir"] == direction].copy().reset_index(drop=True)

        if dir_data.empty:
            print(f"    No data for {direction}")
            continue

        dir_data["From Station"] = dir_data["From Station"].apply(
            normalize_station_name
        )
        dir_data["To Station"] = dir_data["To Station"].apply(normalize_station_name)

        od_matrix = build_od_matrix_from_links(dir_data)
        od_matrices[direction] = od_matrix

        print(f"    {len(od_matrix)} origins with destinations")

    return od_matrices


def od_matrix_to_vector(
    od_matrix: Dict[str, Dict[str, float]], all_stations: List[str]
) -> np.ndarray:
    """Convert OD matrix to a flat probability vector."""
    vector = []

    for origin in all_stations:
        if origin not in od_matrix:
            vector.extend([0.0] * len(all_stations))
            continue

        for destination in all_stations:
            vector.append(od_matrix[origin].get(destination, 0.0))

    return np.array(vector)


def compute_correlation_matrix(
    od_matrices: Dict[str, Dict], day_codes: List[str], all_stations: List[str]
) -> np.ndarray:
    n_days = len(day_codes)
    corr_matrix = np.zeros((n_days, n_days))

    vectors = {}
    for day in day_codes:
        if day in od_matrices and od_matrices[day]:
            vectors[day] = od_matrix_to_vector(od_matrices[day], all_stations)
        else:
            vectors[day] = np.zeros(len(all_stations) ** 2)

    for i, day1 in enumerate(day_codes):
        for j, day2 in enumerate(day_codes):
            if i == j:
                corr_matrix[i, j] = 1.0
            else:
                v1, v2 = vectors[day1], vectors[day2]
                mask = (v1 != 0) | (v2 != 0)
                corr_matrix[i, j] = (
                    np.corrcoef(v1[mask], v2[mask])[0, 1] if mask.any() else 0.0
                )

    return corr_matrix


def print_correlation_matrix(corr_matrix, day_codes, direction, output_file):

    output_file.write(f"{direction} Direction - Correlation Matrix\n")

    output_file.write(f"{'':>6}")
    for day in day_codes:
        output_file.write(f"{day:>8}")
    output_file.write("\n")

    for i, day in enumerate(day_codes):
        output_file.write(f"{day:>6}")
        for j in range(len(day_codes)):
            output_file.write(f"{corr_matrix[i, j]:8.4f}")
        output_file.write("\n")

    output_file.write("\n")


def analyze_correlations(corr_matrix, day_codes, direction, output_file):
    output_file.write(f"\nAnalysis for {direction} direction\n")
    output_file.write("-" * 40 + "\n")

    weekday_idx = [i for i, d in enumerate(day_codes) if d in ["MON", "TWT", "FRI"]]
    weekend_idx = [i for i, d in enumerate(day_codes) if d in ["SAT", "SUN"]]

    if len(weekday_idx) >= 2:
        vals = [corr_matrix[i, j] for i in weekday_idx for j in weekday_idx if i < j]
        output_file.write("Weekday-Weekday correlations\n")
        output_file.write(f"  Mean: {np.mean(vals):.4f}\n")
        output_file.write(f"  Min:  {np.min(vals):.4f}\n")
        output_file.write(f"  Max:  {np.max(vals):.4f}\n\n")

    if len(weekend_idx) == 2:
        val = corr_matrix[weekend_idx[0], weekend_idx[1]]
        output_file.write("Weekend-Weekend correlation (SAT-SUN)\n")
        output_file.write(f"  {val:.4f}\n\n")

    if weekday_idx and weekend_idx:
        vals = [corr_matrix[i, j] for i in weekday_idx for j in weekend_idx]
        output_file.write("Weekday-Weekend correlations\n")
        output_file.write(f"  Mean: {np.mean(vals):.4f}\n")
        output_file.write(f"  Min:  {np.min(vals):.4f}\n")
        output_file.write(f"  Max:  {np.max(vals):.4f}\n\n")


def create_heatmap_plot(corr_nb, corr_sb, day_codes, output_path):
    try:
        import matplotlib.pyplot as plt
        import seaborn as sns

        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))

        sns.heatmap(
            corr_nb,
            annot=True,
            fmt=".3f",
            cmap="RdYlGn",
            vmin=0.7,
            vmax=1.0,
            xticklabels=day_codes,
            yticklabels=day_codes,
            ax=ax1,
        )
        ax1.set_title("Northbound OD Matrix Correlations")

        sns.heatmap(
            corr_sb,
            annot=True,
            fmt=".3f",
            cmap="RdYlGn",
            vmin=0.7,
            vmax=1.0,
            xticklabels=day_codes,
            yticklabels=day_codes,
            ax=ax2,
        )
        ax2.set_title("Southbound OD Matrix Correlations")

        plt.tight_layout()
        plt.savefig(output_path, dpi=300)
        plt.close()

    except ImportError:
        print("matplotlib/seaborn not available; skipping visualisation")


def main():

    print("OD MATRIX CORRELATION ANALYSIS")

    OUTPUT_FOLDER.mkdir(exist_ok=True)

    available_files = []
    for fname, code in INPUT_FILES:
        path = INPUT_FOLDER / fname
        if path.exists():
            print(f"  Found {fname}")
            available_files.append((path, code))
        else:
            print(f"  Missing {fname}")

    if not available_files:
        print("No input files found")
        return

    od_nb, od_sb, days = {}, {}, []

    for path, code in available_files:
        mats = extract_od_matrix_for_day(path, code)

        if "NB" in mats:
            od_nb[code] = mats["NB"]
            with open(OUTPUT_FOLDER / f"{code}_NB.json", "w") as f:
                json.dump(mats["NB"], f, indent=2)

        if "SB" in mats:
            od_sb[code] = mats["SB"]
            with open(OUTPUT_FOLDER / f"{code}_SB.json", "w") as f:
                json.dump(mats["SB"], f, indent=2)

        if mats:
            days.append(code)

    corr_nb = compute_correlation_matrix(od_nb, days, JUBILEE_STATIONS)
    corr_sb = compute_correlation_matrix(od_sb, days, JUBILEE_STATIONS)

    results_path = OUTPUT_FOLDER / "correlation_results.txt"
    with open(results_path, "w") as f:
        f.write("OD MATRIX CORRELATION ANALYSIS\n")

        f.write(f"Days analysed: {', '.join(days)}\n")

        print_correlation_matrix(corr_nb, days, "NORTHBOUND", f)
        analyze_correlations(corr_nb, days, "NORTHBOUND", f)

        print_correlation_matrix(corr_sb, days, "SOUTHBOUND", f)
        analyze_correlations(corr_sb, days, "SOUTHBOUND", f)

    create_heatmap_plot(
        corr_nb,
        corr_sb,
        days,
        OUTPUT_FOLDER / "correlation_heatmaps.png",
    )

    print("Analysis complete")


if __name__ == "__main__":
    main()

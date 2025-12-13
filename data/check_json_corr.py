"""
OD Matrix Correlation Analysis

Extracts individual OD matrices for each day and direction, then computes
pairwise correlations to validate whether OD probabilities are stable across days.

Outputs:
- 10 JSON files in od_json/ (MON_NB.json, MON_SB.json, etc.)
- correlation_results.txt with correlation matrices
- correlation_heatmaps.png visualizing the results
"""

import pandas as pd
import json
import numpy as np
from pathlib import Path
from typing import Dict, List, Tuple
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
OUTPUT_FOLDER = DATA_FOLDER / "od_json"

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
            # Terminal station case
            if dest_idx == len(link_data):
                terminal = link_data.iloc[-1]["To Station"]
                if remaining_load > 0:
                    destinations[terminal] = remaining_load / initial_load
                break

            # Calculate alighters at this station
            load_before = link_data.iloc[dest_idx - 1]["Total"]
            load_after = link_data.iloc[dest_idx]["Total"]
            alighters = max(0, load_before - load_after)

            station = link_data.iloc[dest_idx]["From Station"]

            if alighters > 0:
                destinations[station] = alighters / initial_load
                remaining_load -= alighters

        # Normalize to ensure probabilities sum to 1.0
        total = sum(destinations.values())
        if total > 0:
            destinations = {k: v / total for k, v in destinations.items()}

        od_matrix[origin] = destinations

    return od_matrix


def extract_od_matrix_for_day(file_path: Path, day_code: str) -> Dict[str, Dict]:
    """Extract OD matrices for both directions from a single day's data."""
    print(f"\nProcessing {file_path.name} ({day_code})...")

    df = load_sheet(file_path, "Link_Loads", header_row=2)

    if df.empty:
        return {}

    jubilee = df[df["Line"] == "Jubilee"].copy()

    od_matrices = {}

    for direction in ["NB", "SB"]:
        print(f"  Extracting {direction} OD matrix...")

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
    """
    Convert OD matrix to a flat probability vector for correlation analysis.
    Vector contains all origin-destination pairs in consistent order.
    """
    vector = []

    for origin in all_stations:
        if origin not in od_matrix:
            # Origin not in matrix, pad with zeros
            vector.extend([0.0] * len(all_stations))
            continue

        for destination in all_stations:
            prob = od_matrix[origin].get(destination, 0.0)
            vector.append(prob)

    return np.array(vector)


def compute_correlation_matrix(
    od_matrices: Dict[str, Dict], day_codes: List[str], all_stations: List[str]
) -> np.ndarray:
    """Compute pairwise correlation matrix between OD matrices from different days."""
    n_days = len(day_codes)
    corr_matrix = np.zeros((n_days, n_days))

    # Convert all OD matrices to vectors
    vectors = {}
    for day in day_codes:
        if day in od_matrices and od_matrices[day]:
            vectors[day] = od_matrix_to_vector(od_matrices[day], all_stations)
        else:
            vectors[day] = np.zeros(len(all_stations) ** 2)

    # Compute pairwise correlations
    for i, day1 in enumerate(day_codes):
        for j, day2 in enumerate(day_codes):
            if i == j:
                corr_matrix[i, j] = 1.0
            else:
                vec1 = vectors[day1]
                vec2 = vectors[day2]

                # Remove zero-zero pairs to avoid division by zero
                mask = (vec1 != 0) | (vec2 != 0)
                if mask.sum() > 0:
                    corr = np.corrcoef(vec1[mask], vec2[mask])[0, 1]
                    corr_matrix[i, j] = corr
                else:
                    corr_matrix[i, j] = 0.0

    return corr_matrix


def print_correlation_matrix(
    corr_matrix: np.ndarray, day_codes: List[str], direction: str, output_file
):
    """Print formatted correlation matrix."""
    output_file.write(f"\n{'='*60}\n")
    output_file.write(f"{direction} Direction - Correlation Matrix\n")
    output_file.write(f"{'='*60}\n\n")

    # Header
    output_file.write(f"{'':>6}")
    for day in day_codes:
        output_file.write(f"{day:>8}")
    output_file.write("\n")

    # Rows
    for i, day in enumerate(day_codes):
        output_file.write(f"{day:>6}")
        for j in range(len(day_codes)):
            output_file.write(f"{corr_matrix[i, j]:8.4f}")
        output_file.write("\n")

    output_file.write("\n")


def analyze_correlations(
    corr_matrix: np.ndarray, day_codes: List[str], direction: str, output_file
):
    """Analyze and report correlation patterns."""
    output_file.write(f"\nAnalysis for {direction} direction:\n")
    output_file.write("-" * 40 + "\n")

    # Weekday pairs (MON, TWT, FRI)
    weekday_indices = [
        i for i, day in enumerate(day_codes) if day in ["MON", "TWT", "FRI"]
    ]
    if len(weekday_indices) >= 2:
        weekday_corrs = []
        for i in range(len(weekday_indices)):
            for j in range(i + 1, len(weekday_indices)):
                weekday_corrs.append(
                    corr_matrix[weekday_indices[i], weekday_indices[j]]
                )

        if weekday_corrs:
            output_file.write(f"Weekday-Weekday correlations:\n")
            output_file.write(f"  Mean: {np.mean(weekday_corrs):.4f}\n")
            output_file.write(f"  Min:  {np.min(weekday_corrs):.4f}\n")
            output_file.write(f"  Max:  {np.max(weekday_corrs):.4f}\n\n")

    # Weekend pairs (SAT, SUN)
    weekend_indices = [i for i, day in enumerate(day_codes) if day in ["SAT", "SUN"]]
    if len(weekend_indices) == 2:
        weekend_corr = corr_matrix[weekend_indices[0], weekend_indices[1]]
        output_file.write(f"Weekend-Weekend correlation (SAT-SUN):\n")
        output_file.write(f"  {weekend_corr:.4f}\n\n")

    # Weekday-Weekend pairs
    if weekday_indices and weekend_indices:
        weekday_weekend_corrs = []
        for i in weekday_indices:
            for j in weekend_indices:
                weekday_weekend_corrs.append(corr_matrix[i, j])

        if weekday_weekend_corrs:
            output_file.write(f"Weekday-Weekend correlations:\n")
            output_file.write(f"  Mean: {np.mean(weekday_weekend_corrs):.4f}\n")
            output_file.write(f"  Min:  {np.min(weekday_weekend_corrs):.4f}\n")
            output_file.write(f"  Max:  {np.max(weekday_weekend_corrs):.4f}\n\n")

    # Recommendation
    output_file.write("Recommendation:\n")
    if weekday_corrs and np.mean(weekday_corrs) > 0.95:
        output_file.write("  ✓ Weekdays are highly consistent (>0.95)\n")
        output_file.write("    → Single weekday OD matrix is justified\n")
    else:
        output_file.write("  ⚠ Weekday variation detected\n")
        output_file.write("    → Consider separate matrices per weekday\n")

    if weekend_indices and len(weekend_indices) == 2:
        if weekend_corr > 0.90:
            output_file.write("  ✓ Weekend days are consistent (>0.90)\n")
            output_file.write("    → Single weekend OD matrix is justified\n")
        else:
            output_file.write("  ⚠ Weekend day variation detected\n")
            output_file.write("    → Consider separate SAT/SUN matrices\n")

    output_file.write("\n")


def create_heatmap_plot(
    corr_nb: np.ndarray, corr_sb: np.ndarray, day_codes: List[str], output_path: Path
):
    """Create visualization of correlation matrices."""
    try:
        import matplotlib.pyplot as plt
        import seaborn as sns

        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))

        # NB heatmap
        sns.heatmap(
            corr_nb,
            annot=True,
            fmt=".3f",
            cmap="RdYlGn",
            vmin=0.7,
            vmax=1.0,
            center=0.85,
            xticklabels=day_codes,
            yticklabels=day_codes,
            ax=ax1,
            cbar_kws={"label": "Correlation"},
        )
        ax1.set_title(
            "Northbound OD Matrix Correlations", fontsize=14, fontweight="bold"
        )

        # SB heatmap
        sns.heatmap(
            corr_sb,
            annot=True,
            fmt=".3f",
            cmap="RdYlGn",
            vmin=0.7,
            vmax=1.0,
            center=0.85,
            xticklabels=day_codes,
            yticklabels=day_codes,
            ax=ax2,
            cbar_kws={"label": "Correlation"},
        )
        ax2.set_title(
            "Southbound OD Matrix Correlations", fontsize=14, fontweight="bold"
        )

        plt.tight_layout()
        plt.savefig(output_path, dpi=300, bbox_inches="tight")
        print(f"\n✓ Saved visualization: {output_path}")

    except ImportError:
        print("\n⚠ matplotlib/seaborn not available - skipping visualization")


def main():
    print("=" * 60)
    print("OD MATRIX CORRELATION ANALYSIS")
    print("=" * 60)

    OUTPUT_FOLDER.mkdir(exist_ok=True)

    print("\nChecking input files...")
    available_files = []

    for fname, day_code in INPUT_FILES:
        file_path = INPUT_FOLDER / fname
        if file_path.exists():
            print(f"  ✓ {fname}")
            available_files.append((file_path, day_code))
        else:
            print(f"  ✗ {fname} (not found)")

    if not available_files:
        print("\n❌ ERROR: No input files found in raw_excel/ folder!")
        return

    print(f"\n{len(available_files)} file(s) available for processing\n")

    # Extract OD matrices for each day
    od_matrices_nb = {}
    od_matrices_sb = {}
    day_codes_found = []

    for file_path, day_code in available_files:
        matrices = extract_od_matrix_for_day(file_path, day_code)

        if "NB" in matrices:
            od_matrices_nb[day_code] = matrices["NB"]
            # Save individual JSON
            output_path = OUTPUT_FOLDER / f"{day_code}_NB.json"
            with open(output_path, "w") as f:
                json.dump(matrices["NB"], f, indent=2)
            print(f"  Saved: {output_path}")

        if "SB" in matrices:
            od_matrices_sb[day_code] = matrices["SB"]
            # Save individual JSON
            output_path = OUTPUT_FOLDER / f"{day_code}_SB.json"
            with open(output_path, "w") as f:
                json.dump(matrices["SB"], f, indent=2)
            print(f"  Saved: {output_path}")

        if matrices:
            day_codes_found.append(day_code)

    if not day_codes_found:
        print("\n❌ ERROR: No OD matrices extracted!")
        return

    print(f"\n✓ Extracted OD matrices for {len(day_codes_found)} days")
    print(f"  Days: {', '.join(day_codes_found)}")

    # Compute correlation matrices
    print("\n" + "=" * 60)
    print("COMPUTING CORRELATIONS")
    print("=" * 60)

    all_stations = JUBILEE_STATIONS

    corr_matrix_nb = compute_correlation_matrix(
        od_matrices_nb, day_codes_found, all_stations
    )
    corr_matrix_sb = compute_correlation_matrix(
        od_matrices_sb, day_codes_found, all_stations
    )

    # Write results to file
    results_path = OUTPUT_FOLDER / "correlation_results.txt"
    with open(results_path, "w") as f:
        f.write("OD MATRIX CORRELATION ANALYSIS\n")
        f.write("=" * 60 + "\n")
        f.write(f"Analysis Date: {pd.Timestamp.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write(f"Days analyzed: {', '.join(day_codes_found)}\n")

        print_correlation_matrix(corr_matrix_nb, day_codes_found, "NORTHBOUND", f)
        analyze_correlations(corr_matrix_nb, day_codes_found, "NORTHBOUND", f)

        print_correlation_matrix(corr_matrix_sb, day_codes_found, "SOUTHBOUND", f)
        analyze_correlations(corr_matrix_sb, day_codes_found, "SOUTHBOUND", f)

    print(f"\n✓ Saved results: {results_path}")

    # Print to console as well
    with open(results_path, "r") as f:
        print("\n" + f.read())

    # Create visualization
    viz_path = OUTPUT_FOLDER / "correlation_heatmaps.png"
    create_heatmap_plot(corr_matrix_nb, corr_matrix_sb, day_codes_found, viz_path)

    print("\n" + "=" * 60)
    print("ANALYSIS COMPLETE")
    print("=" * 60)
    print(f"\nOutput files in {OUTPUT_FOLDER}/:")
    print(f"  - {len(day_codes_found)*2} individual OD JSON files (DAY_DIRECTION.json)")
    print(f"  - correlation_results.txt (detailed analysis)")
    print(f"  - correlation_heatmaps.png (visualization)")
    print()


if __name__ == "__main__":
    main()

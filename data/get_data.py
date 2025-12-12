"""
NUMBAT Data Extraction for Jubilee Line Simulation

Extracts:
1. boarders_jubilee_nb.csv - Boarders per station (Northbound)
2. boarders_jubilee_sb.csv - Boarders per station (Southbound)
3. interchange_to_jubilee_nb.csv - Aggregated interchange to Jubilee NB
4. interchange_to_jubilee_sb.csv - Aggregated interchange to Jubilee SB
5. od_matrix_nb.json - O-D probabilities northbound
6. od_matrix_sb.json - O-D probabilities southbound
"""

import pandas as pd
import json
from pathlib import Path
from typing import Dict, List, Tuple
import warnings

warnings.filterwarnings("ignore")

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
OUTPUT_FOLDER = DATA_FOLDER / "data_filtered"

INPUT_FILES = [
    "NBT23MON_outputs.xlsx",
    "NBT23TWT_outputs.xlsx",
    "NBT23FRI_outputs.xlsx",
    "NBT23SAT_outputs.xlsx",
    "NBT23SUN_outputs.xlsx",
]

INPUT_FILES = [INPUT_FOLDER / file for file in INPUT_FILES]


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


def get_time_columns(df: pd.DataFrame) -> List[str]:
    return [
        col
        for col in df.columns
        if isinstance(col, str) and "-" in col and len(col) == 9
    ]


def get_boarders(
    input_files: List[Path], output_folder: Path
) -> Tuple[pd.DataFrame, pd.DataFrame]:
    """
    Extract boarders as TWO separate CSVs (NB and SB).
    Each CSV: Station, Total, <time_columns>
    One row per station.
    """

    print("EXTRACTING BOARDERS DATA (Jubilee Line Only)")

    all_boarders = []

    for file_path in input_files:
        if not file_path.exists():
            print(f"Skipping {file_path.name} (not found)")
            continue

        print(f"\nProcessing {file_path.name}...")

        df = load_sheet(file_path, "Station_Boarders", header_row=2)

        if df.empty:
            continue

        jubilee_boarders = df[df["Line"] == "JUB"].copy()
        jubilee_boarders["Station"] = jubilee_boarders["Station"].apply(
            normalize_station_name
        )

        # Keep Platform temporarily to split, then drop it
        cols_to_keep = ["Station", "Platform", "Total"] + get_time_columns(df)
        jubilee_boarders = jubilee_boarders[cols_to_keep]

        jubilee_boarders["Source"] = file_path.name
        all_boarders.append(jubilee_boarders)

        print(f"  Found {len(jubilee_boarders)} Jubilee boarder records")

    if not all_boarders:
        print("\nNo boarders data found")
        return pd.DataFrame(), pd.DataFrame()

    combined = pd.concat(all_boarders, ignore_index=True)

    # Split into NB and SB
    time_cols = get_time_columns(combined)

    # Northbound
    nb_boarders = combined[combined["Platform"].str.contains("NB", na=False)].copy()
    nb_boarders = nb_boarders.drop(columns=["Platform", "Source"])

    output_nb = output_folder / "boarders_jubilee_nb.csv"
    nb_boarders.to_csv(output_nb, index=False)

    print(f"\n Saved: {output_nb}")
    print(f"  Stations: {len(nb_boarders)}")

    # Southbound
    sb_boarders = combined[combined["Platform"].str.contains("SB", na=False)].copy()
    sb_boarders = sb_boarders.drop(columns=["Platform", "Source"])

    output_sb = output_folder / "boarders_jubilee_sb.csv"
    sb_boarders.to_csv(output_sb, index=False)

    print(f"\n Saved: {output_sb}")
    print(f"  Stations: {len(sb_boarders)}")

    # Print top stations
    print("\n  Top 5 boarding stations (NB):")
    for _, row in nb_boarders.nlargest(5, "Total").iterrows():
        print(f"    {row['Station']:25} : {row['Total']:10,.0f} passengers/day")

    return nb_boarders, sb_boarders


def get_flows(
    input_files: List[Path], output_folder: Path
) -> Tuple[pd.DataFrame, pd.DataFrame]:
    """
    Extract interchange flows as TWO separate CSVs (NB and SB).
    Each CSV: Station, Total, <time_columns>
    Already aggregated across all origin lines.
    """

    print("EXTRACTING INTERCHANGE FLOWS (To Jubilee Line)")

    all_flows = []

    for file_path in input_files:
        if not file_path.exists():
            print(f"Skipping {file_path.name} (not found)")
            continue

        print(f"\nProcessing {file_path.name}...")

        df = load_sheet(file_path, "Station_Flows", header_row=2)

        if df.empty:
            continue

        # Filter for interchange TO Jubilee
        interchange_to_jubilee = df[
            (df["Movement"] == "Alight-Interchange-Board")
            & (df["To Node"].str.contains("Jubilee", na=False))
        ].copy()

        interchange_to_jubilee["From Station"] = interchange_to_jubilee[
            "From Station"
        ].apply(normalize_station_name)

        # Keep From Station, To Node, Total, and time columns
        cols_to_keep = ["From Station", "To Node", "Total"] + get_time_columns(df)

        interchange_to_jubilee = interchange_to_jubilee[cols_to_keep]
        interchange_to_jubilee = interchange_to_jubilee.rename(
            columns={"From Station": "Station"}
        )

        interchange_to_jubilee["Source"] = file_path.name
        all_flows.append(interchange_to_jubilee)

        print(f"  Found {len(interchange_to_jubilee)} interchange records")

    if not all_flows:
        print("\nNo interchange flows data found")
        return pd.DataFrame(), pd.DataFrame()

    combined = pd.concat(all_flows, ignore_index=True)

    # Split into NB and SB, aggregate across all origin lines
    time_cols = get_time_columns(combined)

    # Northbound
    nb_flows = combined[combined["To Node"].str.contains("NB", na=False)].copy()
    nb_aggregated = (
        nb_flows.groupby("Station")[["Total"] + time_cols].sum().reset_index()
    )

    output_nb = output_folder / "interchange_to_jubilee_nb.csv"
    nb_aggregated.to_csv(output_nb, index=False)

    print(f"\n Saved: {output_nb}")
    print(f"  Stations: {len(nb_aggregated)}")

    # Southbound
    sb_flows = combined[combined["To Node"].str.contains("SB", na=False)].copy()
    sb_aggregated = (
        sb_flows.groupby("Station")[["Total"] + time_cols].sum().reset_index()
    )

    output_sb = output_folder / "interchange_to_jubilee_sb.csv"
    sb_aggregated.to_csv(output_sb, index=False)

    print(f"\n Saved: {output_sb}")
    print(f"  Stations: {len(sb_aggregated)}")

    # Print top stations
    print("\n  Top 5 interchange stations (NB):")
    for _, row in nb_aggregated.nlargest(5, "Total").iterrows():
        print(f"    {row['Station']:25} : {row['Total']:10,.0f} passengers/day")

    return nb_aggregated, sb_aggregated


def get_odmatrix(input_files: List[Path], output_folder: Path) -> Tuple[Dict, Dict]:
    """
    Derive O-D matrix from Link_Loads.
    Returns two JSONs: od_matrix_nb.json and od_matrix_sb.json
    """

    print("DERIVING O-D MATRIX (From Link Loads)")

    primary_names = {"NBT23MON_outputs.xlsx", "NBT23TWT_outputs.xlsx"}
    primary_files = [p for p in input_files if p.name in primary_names]

    if not primary_files:
        print("No weekday files found (need MON or TWT)")
        return {}, {}

    od_matrices = {"NB": {}, "SB": {}}

    for file_path in primary_files:
        if not file_path.exists():
            print(f"Skipping {file_path.name} (not found)")
            continue

        print(f"\nProcessing {file_path.name}...")

        df = load_sheet(file_path, "Link_Loads", header_row=2)

        if df.empty:
            continue

        jubilee = df[df["Line"] == "Jubilee"].copy()

        for direction in ["NB", "SB"]:
            print(f"  Building O-D matrix for {direction}...")

            dir_data = (
                jubilee[jubilee["Dir"] == direction].copy().reset_index(drop=True)
            )

            if dir_data.empty:
                continue

            dir_data["From Station"] = dir_data["From Station"].apply(
                normalize_station_name
            )
            dir_data["To Station"] = dir_data["To Station"].apply(
                normalize_station_name
            )

            od_matrix = build_od_matrix_from_links(dir_data)

            if not od_matrices[direction]:
                od_matrices[direction] = od_matrix
            else:
                od_matrices[direction] = merge_od_matrices(
                    od_matrices[direction], od_matrix
                )

            print(f"     {len(od_matrix)} origins")

    # Save to JSON
    for direction, matrix in od_matrices.items():
        if not matrix:
            continue

        output_path = output_folder / f"od_matrix_{direction.lower()}.json"

        with open(output_path, "w") as f:
            json.dump(matrix, f, indent=2)

        print(f"\n Saved: {output_path}")
        print(f"  Origins: {len(matrix)}")

        # Sample
        sample_origin = list(matrix.keys())[0]
        print(f"  Sample (from {sample_origin}):")
        for dest, prob in list(matrix[sample_origin].items())[:3]:
            print(f"    -> {dest:25} : {prob:6.2%}")

    return od_matrices["NB"], od_matrices["SB"]


def build_od_matrix_from_links(link_data: pd.DataFrame) -> Dict[str, Dict[str, float]]:
    """Build O-D probability matrix from link load data"""
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


def merge_od_matrices(matrix1: Dict, matrix2: Dict) -> Dict:
    """Average two O-D matrices"""
    merged = {}

    all_origins = set(matrix1.keys()) | set(matrix2.keys())

    for origin in all_origins:
        merged[origin] = {}

        dests1 = matrix1.get(origin, {})
        dests2 = matrix2.get(origin, {})

        all_dests = set(dests1.keys()) | set(dests2.keys())

        for dest in all_dests:
            merged[origin][dest] = (dests1.get(dest, 0) + dests2.get(dest, 0)) / 2

    return merged


def main():

    print("NUMBAT DATA EXTRACTION FOR JUBILEE LINE SIMULATION")

    OUTPUT_FOLDER.mkdir(exist_ok=True)

    print("\nChecking input files...")
    available_files = []

    for file_path in INPUT_FILES:
        if file_path.exists():
            print(f"   {file_path.name}")
            available_files.append(file_path)
        else:
            print(f"  ✗ {file_path.name} (not found)")

    if not available_files:
        print("\n ERROR: No input files found in raw_excel/ folder!")
        return

    print(f"\n  Found {len(available_files)} file(s) to process")

    try:
        nb_boarders, sb_boarders = get_boarders(available_files, OUTPUT_FOLDER)
        nb_flows, sb_flows = get_flows(available_files, OUTPUT_FOLDER)
        od_nb, od_sb = get_odmatrix(available_files, OUTPUT_FOLDER)

        print("EXTRACTION COMPLETE")

        print("\n Output files:")
        print("  1. boarders_jubilee_nb.csv")
        print("  2. boarders_jubilee_sb.csv")
        print("  3. interchange_to_jubilee_nb.csv")
        print("  4. interchange_to_jubilee_sb.csv")
        print("  5. od_matrix_nb.json")
        print("  6. od_matrix_sb.json")

    except Exception as e:
        print(f"\n ERROR: {e}")
        import traceback

        traceback.print_exc()


if __name__ == "__main__":
    main()

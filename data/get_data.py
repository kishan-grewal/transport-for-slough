"""
NUMBAT Data Extraction for Jubilee Line Simulation

Extracts:
1. boarders.csv - Boarders per station (all days and directions)
2. interchange.csv - Aggregated interchange to Jubilee (all days and directions)
3. od_matrix.json - O-D probabilities (Direction -> Day -> Origin -> Destinations)
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
OUTPUT_FOLDER = DATA_FOLDER

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


def get_time_columns(df: pd.DataFrame) -> List[str]:
    return [
        col
        for col in df.columns
        if isinstance(col, str) and "-" in col and len(col) == 9
    ]


def get_boarders(file_path: Path, day_code: str) -> pd.DataFrame:
    print(f"\nProcessing {file_path.name}...")

    df = load_sheet(file_path, "Station_Boarders", header_row=2)

    if df.empty:
        return pd.DataFrame()

    jubilee_boarders = df[df["Line"] == "JUB"].copy()
    jubilee_boarders["Station"] = jubilee_boarders["Station"].apply(
        normalize_station_name
    )

    time_cols = get_time_columns(df)
    cols_to_keep = ["Station", "Platform", "Total"] + time_cols
    jubilee_boarders = jubilee_boarders[cols_to_keep]

    nb_boarders = jubilee_boarders[
        jubilee_boarders["Platform"].str.contains("NB", na=False)
    ].copy()
    nb_boarders.insert(1, "Direction", "NB")
    nb_boarders.insert(2, "Day", day_code)
    nb_boarders = nb_boarders.drop(columns=["Platform"])

    sb_boarders = jubilee_boarders[
        jubilee_boarders["Platform"].str.contains("SB", na=False)
    ].copy()
    sb_boarders.insert(1, "Direction", "SB")
    sb_boarders.insert(2, "Day", day_code)
    sb_boarders = sb_boarders.drop(columns=["Platform"])

    combined = pd.concat([nb_boarders, sb_boarders], ignore_index=True)

    print(f"  Records: {len(combined)}")

    return combined


def get_flows(file_path: Path, day_code: str) -> pd.DataFrame:
    print(f"\nProcessing {file_path.name}...")

    df = load_sheet(file_path, "Station_Flows", header_row=2)

    if df.empty:
        return pd.DataFrame()

    interchange_to_jubilee = df[
        (df["Movement"] == "Alight-Interchange-Board")
        & (df["To Node"].str.contains("Jubilee", na=False))
    ].copy()

    interchange_to_jubilee["From Station"] = interchange_to_jubilee[
        "From Station"
    ].apply(normalize_station_name)

    time_cols = get_time_columns(df)
    cols_to_keep = ["From Station", "To Node", "Total"] + time_cols

    interchange_to_jubilee = interchange_to_jubilee[cols_to_keep]
    interchange_to_jubilee = interchange_to_jubilee.rename(
        columns={"From Station": "Station"}
    )

    nb_flows = interchange_to_jubilee[
        interchange_to_jubilee["To Node"].str.contains("NB", na=False)
    ].copy()
    nb_aggregated = (
        nb_flows.groupby("Station")[["Total"] + time_cols].sum().reset_index()
    )
    nb_aggregated.insert(1, "Direction", "NB")
    nb_aggregated.insert(2, "Day", day_code)

    sb_flows = interchange_to_jubilee[
        interchange_to_jubilee["To Node"].str.contains("SB", na=False)
    ].copy()
    sb_aggregated = (
        sb_flows.groupby("Station")[["Total"] + time_cols].sum().reset_index()
    )
    sb_aggregated.insert(1, "Direction", "SB")
    sb_aggregated.insert(2, "Day", day_code)

    combined = pd.concat([nb_aggregated, sb_aggregated], ignore_index=True)

    print(f"  Records: {len(combined)}")

    return combined


def get_odmatrix(input_files: List[Tuple[str, str]]) -> Dict:
    print("DERIVING O-D MATRIX (From Link Loads)")

    od_matrices = {"NB": {}, "SB": {}}

    for fname, day_code in input_files:
        file_path = INPUT_FOLDER / fname

        if not file_path.exists():
            print(f"Skipping {fname} (not found)")
            continue

        print(f"\nProcessing {fname}...")

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

            if day_code not in od_matrices[direction]:
                od_matrices[direction][day_code] = {}

            od_matrices[direction][day_code] = od_matrix

            print(f"     {len(od_matrix)} origins")

    return od_matrices


def build_od_matrix_from_links(link_data: pd.DataFrame) -> Dict[str, Dict[str, float]]:
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


def main():

    print("NUMBAT DATA EXTRACTION FOR JUBILEE LINE SIMULATION")

    OUTPUT_FOLDER.mkdir(exist_ok=True)

    print("\nChecking input files...")
    available_files = []

    for fname, day_code in INPUT_FILES:
        file_path = INPUT_FOLDER / fname
        if file_path.exists():
            print(f"   {fname}")
            available_files.append((file_path, day_code))
        else:
            print(f"   {fname} (not found)")

    if not available_files:
        print("\n ERROR: No input files found in raw_excel/ folder!")
        return

    print(f"\n  Found {len(available_files)} file(s) to process")

    try:
        print("EXTRACTING BOARDERS DATA (Jubilee Line Only)")
        all_boarders = []
        for file_path, day_code in available_files:
            df = get_boarders(file_path, day_code)
            if not df.empty:
                all_boarders.append(df)

        if all_boarders:
            boarders_combined = pd.concat(all_boarders, ignore_index=True)
            output_path = OUTPUT_FOLDER / "boarders.csv"
            boarders_combined.to_csv(output_path, index=False)
            print(f"\n Saved: {output_path}")
            print(f"  Total records: {len(boarders_combined)}")

        print("EXTRACTING INTERCHANGE FLOWS (To Jubilee Line)")
        all_interchange = []
        for file_path, day_code in available_files:
            df = get_flows(file_path, day_code)
            if not df.empty:
                all_interchange.append(df)

        if all_interchange:
            interchange_combined = pd.concat(all_interchange, ignore_index=True)
            output_path = OUTPUT_FOLDER / "interchange.csv"
            interchange_combined.to_csv(output_path, index=False)
            print(f"\n Saved: {output_path}")
            print(f"  Total records: {len(interchange_combined)}")

        od_matrix = get_odmatrix(INPUT_FILES)

        if od_matrix:
            output_path = OUTPUT_FOLDER / "od_matrix.json"
            with open(output_path, "w") as f:
                json.dump(od_matrix, f, indent=2)
            print(f"\n Saved: {output_path}")
            print(f"  NB days: {len(od_matrix.get('NB', {}))}")
            print(f"  SB days: {len(od_matrix.get('SB', {}))}")

        print("EXTRACTION COMPLETE")

        print("\n Output files:")
        print("  boarders.csv")
        print("  interchange.csv")
        print("  od_matrix.json")

    except Exception as e:
        print(f"\n ERROR: {e}")
        import traceback

        traceback.print_exc()


if __name__ == "__main__":
    main()

import pandas as pd
from pathlib import Path

data_folder = Path("excel_filtered")
csv_day_folder = Path("csv_day")
csv_sheet_folder = Path("csv_sheet")

csv_day_folder.mkdir(exist_ok=True)
csv_sheet_folder.mkdir(exist_ok=True)

sheet_names = [
    "Station_Flows",
    "Station_Entries",
    "Station_Exits",
    "Station_Boarders",
    "Station_Alighters",
]

for excel_path in data_folder.glob("*.xlsx"):
    base_name = excel_path.stem  # e.g. NBT23MON_filtered
    print(f"Processing {base_name}...")

    # Extract day code (MON, TWT, etc.)
    day_code = base_name.split("_")[0].replace("NBT23", "")

    # Create csv_day/<DAY>/ folder
    day_output_folder = csv_day_folder / day_code
    day_output_folder.mkdir(parents=True, exist_ok=True)

    # Read all sheets from Excel file
    xls = pd.read_excel(excel_path, sheet_name=sheet_names)

    for sheet_name, df in xls.items():
        # 1️⃣ Save into csv_day/<DAY>/
        day_csv = day_output_folder / f"{base_name}_{sheet_name}.csv"
        df.to_csv(day_csv, index=False)

        # 2️⃣ Save into csv_sheet/<SHEET>/
        sheet_folder = csv_sheet_folder / sheet_name
        sheet_folder.mkdir(parents=True, exist_ok=True)
        sheet_csv = sheet_folder / f"{base_name}.csv"
        df.to_csv(sheet_csv, index=False)

        print(f"    Saved {day_csv}")
        print(f"    Saved {sheet_csv}")

print("All files processed successfully")

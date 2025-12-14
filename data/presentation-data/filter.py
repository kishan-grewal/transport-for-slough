import pandas as pd
from pathlib import Path

jubilee_stations = [
    "Stratford",
    "West Ham",
    "Canning Town",
    "North Greenwich",
    "Canary Wharf",
    "Canada Water",
    "Bermondsey",
    "London Bridge",
    "Southwark",
    "Waterloo",
    "Westminster",
    "Green Park",
    "Bond Street",
    "Baker Street",
    "St. John's Wood",
    "Swiss Cottage",
    "Finchley Road",
    "West Hampstead",
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

sheet_names = [
    "Station_Flows",
    "Station_Entries",
    "Station_Exits",
    "Station_Boarders",
    "Station_Alighters",
]

data_folder = Path("raw_excel")
output_folder = Path("excel_filtered")
output_folder.mkdir(exist_ok=True)

# input_files = [
#     "NBT23MON_outputs.xlsx",
#     "NBT23TWT_outputs.xlsx",
#     "NBT23FRI_outputs.xlsx",
#     "NBT23SAT_outputs.xlsx",
#     "NBT23SUN_outputs.xlsx",
# ]
input_files = [
    "NBT24MON_outputs.xlsx",
    "NBT24TWT_outputs.xlsx",
    "NBT24FRI_outputs.xlsx",
    "NBT24SAT_outputs.xlsx",
    "NBT24SUN_outputs.xlsx",
]

for file_name in input_files:
    input_path = data_folder / file_name
    print(f"Processing {input_path}...")
    xls = pd.read_excel(
        input_path, sheet_name=sheet_names, header=2
    )  # row 0 and row 1 are junk
    filtered_sheets = {}

    df = xls["Station_Flows"]
    filtered_sheets["Station_Flows"] = df[
        df["From Station"].isin(jubilee_stations)
        | df["To Station"].isin(jubilee_stations)
    ]

    df = xls["Station_Entries"]
    filtered_sheets["Station_Entries"] = df[df["Station"].isin(jubilee_stations)]

    df = xls["Station_Exits"]
    filtered_sheets["Station_Exits"] = df[df["Station"].isin(jubilee_stations)]

    df = xls["Station_Boarders"]
    # filtered_sheets["Station_Boarders"] = df[df["Station"].isin(jubilee_stations)]  # alternate filter by Station
    filtered_sheets["Station_Boarders"] = df[df["Line"] == "JUB"]

    df = xls["Station_Alighters"]
    # filtered_sheets["Station_Alighters"] = df[df["Station"].isin(jubilee_stations)]  # alternate filter by Station
    filtered_sheets["Station_Alighters"] = df[df["Line"] == "JUB"]

    output_path = output_folder / file_name.replace("_outputs", "_filtered")

    with pd.ExcelWriter(output_path) as writer:
        for name, df in filtered_sheets.items():
            df.to_excel(writer, sheet_name=name, index=False)

    print(f"Saved filtered file: {output_path}")

import pandas as pd, requests, pickle

def get_station_ids(line: str) -> list[tuple[str, str, str]]:
  url = f"https://api.tfl.gov.uk/line/{line.lower()}/stoppoints"
  # Using naptan ID csv, since the API is not 100% correct
  naptans = pd.read_csv("data-collection/station_structure/naptan.csv")

  response = requests.get(url).json()
  keys = [' '.join(s["commonName"].split(' ')[:-2]) for s in response]
  return [(str(s["commonName"]),
           str(s["naptanId"]),
           str(naptans[naptans["commonName"].apply(lambda x : x.__contains__(key))]["naptanID"].values[0]),
           )
           for s,key in zip(response, keys)]

# print(*get_station_ids("jubilee"),"\n")


path = "data-collection/station_structure/outputs/stations_mod.pkl"
with open(path, "rb") as f:
  stations = pickle.load(f)
assert stations is not None

print(stations.keys())
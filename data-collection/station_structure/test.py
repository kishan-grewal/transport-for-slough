# import pandas as pd, requests, pickle

# def get_station_ids(line: str) -> list[tuple[str, str, str]]:
#   url = f"https://api.tfl.gov.uk/line/{line.lower()}/stoppoints"
#   # Using naptan ID csv, since the API is not 100% correct
#   naptans = pd.read_csv("data-collection/station_structure/naptan.csv")

#   response = requests.get(url).json()
#   keys = [' '.join(s["commonName"].split(' ')[:-2]) for s in response]
#   return [(str(s["commonName"]),
#            str(s["naptanId"]),
#            str(naptans[naptans["commonName"].apply(lambda x : x.__contains__(key))]["naptanID"].values[0]),
#            )
#            for s,key in zip(response, keys)]

# # print(*get_station_ids("jubilee"),"\n")


# path = "data-collection/station_structure/outputs/stations_mod.pkl"
# with open(path, "rb") as f:
#   stations = pickle.load(f)
# assert stations is not None

# print(stations.keys())

import pickle
import pandas as pd
from station_structure import Station_Structure

with open("data-collection/station_structure/outputs/stations_mod.pkl","rb") as f:
  data = pickle.load(f)

ind = pd.MultiIndex.from_tuples([(1000025,29),(1000025,30)],names=["stopID","area"])
dat = pd.DataFrame([{"level":-6.0,"x":-0.149791,"y":51.514744,"nodeType":""},{"level":-6.0,"x":-0.149660,"y":51.514778,"nodeType":""}],index=ind)
data["Bond Street Underground Station"].nodes = pd.concat([data["Bond Street Underground Station"].nodes,dat])

data["Bond Street Underground Station"].edges = pd.concat([data["Bond Street Underground Station"].edges, pd.DataFrame([{"start":(1000025,29),"end":(1000025, 4),"type":"LEVEL"},{"start":(1000025,30),"end":(1000025, 5),"type":"LEVEL"}])])
print(data["Bond Street Underground Station"].edges)

with open("data-collection/station_structure/outputs/stations_mod2.pkl","wb+") as f:
  pickle.dump(data,f)
import json
import matplotlib.pyplot as plt
import pandas as pd

with open("config/station_config_compressed.json","r") as f:
  station_config = json.load(f)
assert (station_config is not None)

plt.figure()
i = 1
for station in station_config.keys():
  station_trunc = ' '.join(station.split(' ')[:-2])
  
  legend = []
  key_indexes = []; secondary_indexes = []
  for key in station_config[station]:
    if "Entrance" in key or "Platform" in key:
      legend.extend([key + " (1)", key + " (2)"])
      key_indexes.extend(station_config[station][key])
    else:
      secondary_indexes.extend(station_config[station][key])
      
  data = pd.read_csv(f"out/stations/{station_trunc}.csv").dropna(axis=1).to_numpy()
  
  plt.subplot(3,9,i)
  plt.plot(data[:,key_indexes])
  plt.plot(data[:,secondary_indexes],'k--')
  plt.title(station_trunc)
  # plt.legend(legend)
  i += 1
  
plt.show()